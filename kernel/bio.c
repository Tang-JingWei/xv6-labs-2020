// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
#define HASH(num) (num % NBUCKET)

// 哈希桶
struct hashbuf
{
  struct spinlock lock;
  struct buf head;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
  struct hashbuf buckets[NBUCKET];
} bcache;

void
binit(void)
{
  // struct buf *b;

  initlock(&bcache.lock, "bcache");

  for (int i = 0; i < NBUCKET; i++)
  {
    initlock(&bcache.buckets[i].lock, "bcache.bucket");

    bcache.buckets[i].head.next = &bcache.buckets[i].head;
    bcache.buckets[i].head.prev = &bcache.buckets[i].head;
  }

  // 初始化将所有缓冲区挂载在第一个桶中
  for (int i = 0; i < NBUF; i++) {
    if (i == 0) {
      bcache.buf[i].prev = &bcache.buckets[0].head;
      bcache.buf[i].next = &bcache.buf[i+1];
      bcache.buckets[0].head.next = &bcache.buf[i];
    } else if (i == NBUF - 1) {
      bcache.buf[i].next = 0;
      bcache.buf[i].prev = &bcache.buf[i-1];
    } else {
      bcache.buf[i].next = &bcache.buf[i+1];
      bcache.buf[i].prev = &bcache.buf[i-1];
    }
  }
}

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // acquire(&bcache.lock);

  // 哈希计算 bid
  uint bid = HASH(blockno);

  acquire(&bcache.buckets[bid].lock);

  // 遍历哈希桶
  b = bcache.buckets[bid].head.next;
  for (int i = 0; ; i++) {
    if (bcache.buckets[bid].head.next == b) { // 空桶
      break;
    }

    if ((b->blockno == blockno) && (b->dev == dev)) { // 匹配到了
      b->refcnt ++;

      // 记录时间戳
      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);

      release(&bcache.buckets[bid].lock);

      acquiresleep(&b->lock); // TODO ?
      return b;
    }
    
    b = b->next;    
  }

  // 未缓存，尝试找一块缓存
  struct buf *tmp;
  b = 0;
  int i = 0, cycle = 0;
  for (i = bid; cycle < NBUCKET ; i = (i + 1) % NBUCKET) {
    cycle++;
    // 获取锁操作，但是如果是遍历当前桶就不重新获取了
    if (i != bid){
      if(!holding(&bcache.buckets[i].lock))
        acquire(&bcache.buckets[i].lock);
    } else {
      continue;
    }
    
    for (tmp = bcache.buckets[i].head.next; tmp != &bcache.buckets[i].head ; tmp = tmp->next) {
      if (tmp->refcnt == 0 && ((b == 0) || (tmp->timestamp < b->timestamp))) {  // TODO b == 0在第一个判断
        b = tmp;
      } else {
        //
      }
    }

    if (b != 0) {
      if (bid != i) { // 是在其他桶中窃取的话，需要移动缓冲区（重新插入到新的桶中去）
        // 插入到表头
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.buckets[i].lock);

        b->next = bcache.buckets[bid].head.next;
        b->prev = &bcache.buckets[bid].head;
        bcache.buckets[bid].head.next = b;
        b->next->prev = b;
      }

      b->blockno = blockno;
      b->dev = dev;
      b->refcnt ++;
      b->valid = 0;
      
      // 记录时间戳
      acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);
      
      release(&bcache.buckets[bid].lock);
      acquiresleep(&b->lock); // TODO ?
      return b;
    } else {
      if (bid != i) {
        release(&bcache.buckets[i].lock);
      }
    }
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  // acquire(&bcache.lock);
  b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }

  // 释放时更新时间戳
  acquire(&tickslock);
  b->timestamp = ticks;
  release(&tickslock);
  
  // release(&bcache.lock);
}

void
bpin(struct buf *b) {
  uint bid = HASH(b->blockno);

  acquire(&bcache.buckets[bid].lock);
  b->refcnt++;
  release(&bcache.buckets[bid].lock);
}

void
bunpin(struct buf *b) {
  uint bid = HASH(b->blockno);

  acquire(&bcache.buckets[bid].lock);
  b->refcnt--;
  release(&bcache.buckets[bid].lock);
}


