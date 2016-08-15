#ifndef INFINIBAND_H
#define INFINIBAND_H

#include "Resources.h"

struct IBSYNMsg {
  uint16_t lid;
  uint32_t qpn;
  uint32_t psn;
  union ibv_gid gid;
} __attribute__((packed));

class Infiniband{
  public:
    Infiniband(string& dn) : device_name(dn) {
      device = devicelist.get_device(device_name.c_str());
      pd = new ProtectionDomain(device);
    }
    class ProtectionDomain {
      public:
        explicit ProtectionDomain(Device *device)
          : pd(ibv_alloc_pd(device->ctxt))
        {
          if (pd == NULL) {
            std::cerr << __func__ << " failed to allocate infiniband protection domain: " << std::endl;
            assert(0);
          }
        }
        ~ProtectionDomain() {
          int rc = ibv_dealloc_pd(pd);
          if (rc != 0) {
            std::cerr << __func__ << " ibv_dealloc_pd failed: " << std::endl;
          }
        }
        ibv_pd* const pd;
    };

    ProtectionDomain* get_pd() {  return pd; }

  private:
    string device_name;
    DeviceList devicelist;
    Device* device;
//    QueueManager queueManager;
    ProtectionDomain* pd;
};
#endif
