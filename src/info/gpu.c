#include "info.h"
#include "../utils.h"

#include <string.h>

#include <stdlib.h>
#include <sys/wait.h>

#ifdef __APPLE__
#include <sys/utsname.h>
#include "../macos_infos.h"
#else
#ifndef __ANDROID__
#include <pci/pci.h>
#endif // __ANDROID__
#endif // __APPLE__

// get the gpu name(s)
int gpu(char *dest) {
    char *gpus[] = {NULL, NULL, NULL};
    char *end;

    #ifdef __APPLE__
        struct utsname name;
        uname(&name);

        if(!strcmp(name.machine, "x86_64"))
            gpus[0] = get_gpu_string();  // only works on x64
        if(!gpus[0] || strcmp(name.machine, "x86_64")) {     // fallback
            char buf[1024];
            int pipes[2];
            if(pipe(pipes))
                return 1;

            if(!fork()) {
                dup2(pipes[1], STDOUT_FILENO);
                close(pipes[0]);
                close(pipes[1]);
                execlp("/usr/sbin/system_profiler", "system_profiler", "SPDisplaysDataType", NULL);
            }
            close(pipes[1]);
            wait(0);
            size_t bytes = read(pipes[0], buf, 1024);
            close(pipes[0]);

            if(bytes < 1)
                return 1;

            gpus[0] = strstr(buf, "Chipset Model: ");
            if(!gpus[0])
                return 1;
            gpus[0] += 15;
            char *end = strchr(gpus[0], '\n');
            if(!end)
                return 1;
            *end = 0;
        }
    #else
    # ifdef __ANDROID__
        return 1;
    # else
        // based on https://github.com/pciutils/pciutils/blob/master/example.c

        char device_class[256], namebuf[768];
        struct pci_dev *dev;
        struct pci_access *pacc = pci_alloc();		// get the pci_access structure;

        pci_init(pacc);		// initialize the PCI library
        pci_scan_bus(pacc);		// we want to get the list of devices

        int i = 0;

        for(dev=pacc->devices; dev; dev=dev->next)	{ // iterates over all devices
            pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);	// fill in header info

            pci_lookup_name(pacc, device_class, 256, PCI_LOOKUP_CLASS, dev->device_class);
            if(!strcmp(device_class, "VGA compatible controller") || !strcmp(device_class, "3D controller")) {
                // look up the full name of the device
                if(config.gpu_index == 0) {
                    gpus[i] = pci_lookup_name(pacc, namebuf+i*256, 256, PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
                    
                    if(i < 2)
                        ++i;
                    else
                        break;
                }
                else {
                    if(i == config.gpu_index-1) {
                        gpus[0] = pci_lookup_name(pacc, namebuf, 256, PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
                        break;
                    }

                    if(i < 2)
                        ++i;
                    else
                        break;
                }
            }
        }



        pci_cleanup(pacc);  // close everything
        // fallback (will only get 1 gpu)

        char gpu[256];

        if(!gpus[0]) {
            if(config.gpu_index != 0)   // lol why would you choose a non-existing GPU
                return 1;

            int pipes[2];

            if(pipe(pipes))
                return 1;

            if(!fork()) {
                close(pipes[0]);
                dup2(pipes[1], STDOUT_FILENO);
                execlp("lspci", "lspci", "-mm", NULL);
            }
            
            close(pipes[1]);
            char *lspci = malloc(0x2000);
            
            wait(NULL);
            lspci[read(pipes[0], lspci, 0x2000)] = 0;
            close(pipes[0]);

            gpus[0] = strstr(lspci, "3D");
            if(!gpus[0]) {
                gpus[0] = strstr(lspci, "VGA");
                if(!gpus[0]) {
                    free(lspci);
                    return 1;
                }
            }

            for(int i = 0; i < 4; ++i) {
                gpus[0] = strchr(gpus[0], '"');
                if(!gpus[0]) {
                    free(lspci);
                    return 1;
                }
                ++gpus[0];

                /* class" "manufacturer" "name"
                 *  "manufacturer" "name"
                 * manufacturer" "name"
                 *  "name"
                 * name"
                 */
            }

            char *end = strchr(gpus[0], '"');   // name
            if(!end) {
                free(lspci);
                return 1;
            }
            *end = 0;
            
            strncpy(gpu, gpus[0], 255);
            free(lspci);
            gpus[0] = gpu;
        }
    # endif // __ANDROID__
    #endif // __APPLE__

    if(!gpus[0])
        return 1;

    // this next part is just random cleanup
    // also, I'm using end as a random char* - BaD pRaCtIcE aNd CoNfUsInG - lol stfu

    dest[0] = 0;    //  yk it's decent a yk it works
    for(unsigned i = 0; i < sizeof(gpus)/sizeof(gpus[0]) && gpus[i%3]; ++i) {
        if(!(gpu_brand)) {
            if((end = strstr(gpus[i], "Intel ")))
                gpus[i] += 6;
            else if((end = strstr(gpus[i], "AMD ")))
                gpus[i] += 4;
            else if((end = strstr(gpus[i], "Apple ")))
                gpus[i] += 6;
        }

        if((end = strchr(gpus[i], '['))) {   // sometimes the gpu is "Architecture [GPU Name]"
            char *ptr = strchr(end, ']');
            if(ptr) {
                gpus[i] = end+1;
                *ptr = 0;
            }
        if((end = strstr(gpus[i], " Integrated Graphics Controller")))
            *end = 0;
        if((end = strstr(gpus[i], " Rev. ")))
            *end = 0;
        }

        // (finally) writing the GPUs into dest
        if(i > 0)
            strncat(dest, ", ", 256-strlen(dest));
        strncat(dest, gpus[i], 256-strlen(dest));
    }

    return 0;
}