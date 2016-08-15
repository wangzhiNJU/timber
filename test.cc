#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/time.h>
#include <unistd.h>
#include "iInfiniband.h"

void registration(int size, int num);
void copy(int size, int num);

int main() {
  int size = 256 * 3 * 1024;
  int bound = 1024 * 1024;
  int num = 1000;
  while(size < bound) {
    registration(size, num);
    copy(size, num);
    size += 4 * 1024;
    std::cerr << " =================================================================== " << std::endl;
  }
  return 0;
}

void copy(int size, int num) {
  size_t bytes = size * num;
  int page_size = sysconf(_SC_PAGESIZE);
  char* src = (char*)memalign(page_size, size);
  char* dest = (char*)memalign(page_size, bytes);

  struct timeval start, end;
  gettimeofday(&start, NULL);
  for(int i = 0; i < num; ++i) {
    memcpy(dest + i * size, src, size);
  }
  gettimeofday(&end, NULL);
  double copy_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  std::cerr << size /1024 << " KB's average copy time: " << copy_time / num << std::endl;
  free(src);
  free(dest);
}

void registration(int size, int num) {
  string device_name("mlx4_0");
  Infiniband* ib = new Infiniband(device_name);
  Infiniband::ProtectionDomain* pd = ib->get_pd();

  size_t bytes = size * num;
  int page_size = sysconf(_SC_PAGESIZE);
  char* buf = (char*)memalign(page_size, bytes);
  assert(buf);

  ibv_mr* mrs[num];
  struct timeval start, end;
  gettimeofday(&start, NULL);
  for(int i = 0; i < num; ++i) {
    mrs[i] = ibv_reg_mr(pd->pd, buf + i * size, size, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
  }
  gettimeofday(&end, NULL);
  double reg_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);

  gettimeofday(&start, NULL);
  for(int i = 0; i < num; ++i) {
    assert(ibv_dereg_mr(mrs[i]) == 0);
  }
  gettimeofday(&end, NULL);
  double dereg_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  std::cerr << size / 1024 << " KB's average reg time: " << reg_time / num << ", dereg time: " << dereg_time / num << std::endl;
  free(buf);
}
