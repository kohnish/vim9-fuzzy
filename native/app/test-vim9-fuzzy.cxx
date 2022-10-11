#include "vim9_fuzzy_env.h"
#include "b64.h"
#include "fuzzy.h"
#include "mru.h"
#include "search_helper.h"
#include <gtest/gtest.h>
#include <memory>

// ToDo: Fix this
TEST(fuzzy, start_fuzzy_response) {
    size_t l;
    auto *c = "78]: Starting systemd-tmpfiles-clean.service - Cleanup of User's Tempor>Oct 11 18:49:50 shuttle-fedora systemd[678]: Finished systemd-tmpfiles-clean.service - Cleanup of User's Tempor> Oct 11 18:59:40 shuttle-fedora systemd[1]: Starting systemd-tmpfiles-clean.service - Cleanup of Temporary Direc> Oct 11 18:59:40 shuttle-fedora systemd[1]: systemd-tmpfiles-clean.service: Deactivated successfully.  Oct 11 18:59:40 shuttle-fedora systemd[1]: Finished systemd-tmpfiles-clean.service - Cleanup of Temporary Direc> Oct 11 19:19:35 shuttle-fedora kernel: usb 1-4: USB disconnect, device number 3 Oct 11 19:19:35 shuttle-fedora kernel: usb 1-4.1: USB disconnect, device number 5 Oct 11 19:19:36 shuttle-fedora kernel: usb 1-4: new high-speed USB device number 9 using xhci_hcd Oct 11 19:19:36 shuttle-fedora kernel: usb 1-4: New USB device found, idVendor=0409, idProduct=005a, bcdDevice=> Oct 11 19:19:36 shuttle-fedora kernel: usb 1-4: New USB device strings: Mfr=0, Product=0, SerialNumber=0 Oct 11 19:19:36 shuttle-fedora kernel: hub 1-4:1.0: USB hub found Oct 11 19:19:36 shuttle-fedora kernel: hub 1-4:1.0: 3 ports detected Oct 11 19:19:36 shuttle-fedora kernel: usb 1-4.1: new full-speed USB device number 10 using xhci_hcd Oct 11 19:19:36 shuttle-fedora kernel: usb 1-4.1: New USB device found, idVendor=0853, idProduct=0100, bcdDevic> Oct 11 19:19:36 shuttle-fedora kernel: usb 1-4.1: New USB device strings: Mfr=1, Product=2, SerialNumber=0 Oct 11 19:19:36 shuttle-fedora kernel: usb 1-4.1: Product: HHKB Professional Oct 11 19:19:36 shuttle-fedora kernel: usb 1-4.1: Manufacturer: Topre Corporation Oct 11 19:19:36 shuttle-fedora kernel: input: Topre Corporation HHKB Professional as /devices/pci0000:00/0000:0> Oct 11 19:19:36 shuttle-fedora kernel: hid-generic 0003:0853:0100.0006: input,hidraw4: USB HID v1.11 Keyboard [> Oct 11 19:19:36 shuttle-fedora systemd-logind[598]: Watching system buttons on /dev/input/event5 (Topre Corpora> Oct 11 19:19:40 shuttle-fedora kernel: usb 1-2: USB disconnect, device number 2 Oct 11 19:19:45 shuttle-fedora kernel: usb 1-2: new full-speed USB device number 11 using xhci_hcd Oct 11 19:19:45 shuttle-fedora kernel: usb 1-2: New USB device found, idVendor=05ac, idProduct=0265, bcdDevice=> Oct 11 19:19:45 shuttle-fedora kernel: usb 1-2: New USB device strings: Mfr=1, Product=2, SerialNumber=3 Oct 11 19:19:45 shuttle-fedora kernel: usb 1-2: Product: Magic Trackpad Oct 11 19:19:45 shuttle-fedora kernel: usb 1-2: Manufacturer: Apple Inc.  Oct 11 19:19:45 shuttle-fedora kernel: usb 1-2: SerialNumber: CC2947301KQJ5R9AE Oct 11 19:19:45 shuttle-fedora kernel: magicmouse 0003:05AC:0265.0007: fixing up magicmouse battery report desc> Oct 11 19:19:45 shuttle-fedora kernel: input: Apple Inc. Magic Trackpad as /devices/pci0000:00/0000:00:14.0/usb> Oct 11 19:19:45 shuttle-fedora kernel: magicmouse 0003:05AC:0265.0007: input,hiddev96,hidraw0: USB HID v1.10 Mo> Oct 11 19:19:45 shuttle-fedora kernel: input: Apple Inc. Magic Trackpad as /devices/pci0000:00/0000:00:14.0/usb> Oct 11 19:19:45 shuttle-fedora kernel: magicmouse 0003:05AC:0265.0008: input,hiddev97,hidraw1: USB HID v1.10 Mo> Oct 11 19:19:45 shuttle-fedora kernel: magicmouse 0003:05AC:0265.0009: hiddev98,hidraw2: USB HID v1.10 Device [> Oct 11 19:19:45 shuttle-fedora kernel: magicmouse 0003:05AC:0265.000A: hiddev99";
    auto *b = base64_encode((unsigned char *)c, strlen(c), &l);
    size_t l2;
    auto *d = base64_decode(b, l, &l2);
    printf("%s\n", (char *)b);
    printf("%s\n", (char *)d);
    free(b);
    free(d);
    //init_cancel_mutex();

    //file_info_t files_list[] = {
    //    { "path_name", "file_name", 1, 0, 0, 0 },
    //    { "native/app/json_msg_handler.c", "json_msg_handler.c", 1, 0, 0, 0 },
    //};
    //size_t match_num = start_fuzzy_response("1", "file", files_list, 1);
    //ASSERT_EQ(match_num, 0);
    //match_num = start_fuzzy_response("f", "file", files_list, 1);
    //ASSERT_GT(match_num, 0);
    //match_num = start_fuzzy_response("file", "file", files_list, 1);
    //ASSERT_GT(match_num, 0);
    //match_num = start_fuzzy_response("file name", "file", files_list, 1);
    //ASSERT_GT(match_num, 0);
    //match_num = start_fuzzy_response("i", "file", files_list, 1);
    //ASSERT_GT(match_num, 0);
    //match_num = start_fuzzy_response("l", "file", files_list, 1);
    //ASSERT_GT(match_num, 0);
    ////match_num = start_fuzzy_response("fe", "file", files_list, 1);
    //match_num = start_fuzzy_response("name", "file", files_list, 1);
    //match_num = start_fuzzy_response("file_name", "file", files_list, 1);
    //match_num = start_fuzzy_response("handler", "file", files_list, 2);
    //match_num = start_fuzzy_response("jsonmsg", "file", files_list, 2);
    //match_num = start_fuzzy_response("jsonmsghandler", "file", files_list, 2);

    //deinit_cancel_mutex();
}
