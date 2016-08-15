#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#define <vector>

#define HUGE_PAGE_SIZE (2 * 1024 * 1024)
#define ALIGN_TO_PAGE_SIZE(x) \
  (((x) + HUGE_PAGE_SIZE -1) / HUGE_PAGE_SIZE * HUGE_PAGE_SIZE)

class MemoryManager {
  public:
    class Chunk {
      public:
        Chunk(char* b, uint32_t len, ibv_mr* m) : buffer(b), capacity(len), offset(0), mr(m) {}
        ~Chunk() {
          assert(ibv_dereg_mr(mr) == 0);
        }
      private:
        char* buffer;
        uint32_t capacity;
        uint32_t bound;
        size_t offset;
        ibv_mr* mr;
        uint64_t owner;
    };
    class Cluster {
      public:
        Cluster(MemoryManager& m, uint32_t s, uint32_t n, bool ehp == false) : memory_manager(m), chunk_size(s), size(n), enabled_huge_page(ehp) {
          capacity = size * chunk_size;
          assert(allocate(size) == 0);
        }
        int allocate(uint32_t s) {
          if(enabled_huge_page) {
            int page_size = sysconf(_SC_PAGESIZE);
            base = (char*)memalign(page_size, bytes);
          }
          else {
            base = (char*)manager.malloc_huge_pages(bytes);
          }
          assert(base);

          for(uint32_t offset = 0; offset < capacity; offset += chunk_size){
            ibv_mr* m = ibv_reg_mr(manager.pd->pd, base+offset, chunk_size, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
            assert(m);
            Chunk* c = new Chunk(base+offset,chunk_size,m);
            free_chunks.push_back(c);
            total_chunks.insert(c);
          }

        }
      private:
        MemoryManager& memory_manager;
        uint32_t chunk_size;
        iuint32_t size;
        uint64_t capacity;
        char* base;
        bool enabled_huge_page;
        vector<Chunk*> free_chunks;
        set<Chunk*> total_chunks;
    };

    void * malloc_huge_pages(size_t size)
    {
      size_t real_size = ALIGN_TO_PAGE_SIZE(size + HUGE_PAGE_SIZE);
      char *ptr = (char *)mmap(NULL, real_size, PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS |MAP_POPULATE | MAP_HUGETLB,-1, 0);
      if (ptr == MAP_FAILED) {
        lderr(cct) << __func__ << " MAP_FAILED" << dendl;
        ptr = (char *)malloc(real_size);
        if (ptr == NULL) return NULL;
        real_size = 0;
      }
      *((size_t *)ptr) = real_size;
      lderr(cct) << __func__ << " bingo!" << dendl;
      return ptr + HUGE_PAGE_SIZE;
    }
    void free_huge_pages(void *ptr)
    {
      if (ptr == NULL) return;
      void *real_ptr = (char *)ptr -HUGE_PAGE_SIZE;
      size_t real_size = *((size_t *)real_ptr);
      assert(real_size % HUGE_PAGE_SIZE == 0);
      if (real_size != 0)
        munmap(real_ptr, real_size);
      else
        free(real_ptr);
    }
};
#endif
