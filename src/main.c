#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "info.h"
#include "utils.h"
#include "logos.h"
#include "queue.h"

/* TODO:
 * !!! option to choose what order the infos are printed in ( modules {"a", "b"} in albafetch.conf)
 * --ascii for custom ascii art (conflicts with --logo) - work in progress
 * more config options (for the single functions)
 */

// various configurations for stuff

// This contains the default config values
struct Config config = {
    false,  // align
    true,   // bold
    NULL,   // logo
    "",     // color
    ": ",   // dash

    true,   // title_color
    true,   // os_arch
    false,  // de_type
    false,  // shell_path
    true,   // cpu_brand
    true,   // cpu_freq
    true,   // cpu_count
    false,  // gpu_brand
    true,   // mem_perc
    true,   // pkg_mgr
    true,   // pkg_pacman
    true,   // pkg_dpkg
    true,   // pkg_rpm
    true,   // pkg_flatpak
    true,   // pkg_snap
    false,  // pkg_pip
    true,   // pkg_brew
    false,  // loc_docker
    false,  // loc_localdomain
    3,      // col_block_len

    "",         // separator_prefix
    "",         // spacing_prefix
    "",         // title_prefix
    "User",     // user_prefix
    "Hostname", // hostname_prefix
    "Uptime",   // uptime_prefix
    "OS",       // os_prefix
    "Kernel",   // kernel_prefix
    "Desktop",  // desktop_prefix
    "Shell",    // shell_prefix
    "Login",    // login_shell_prefixix
    "Terminal", // term_prefix
    "Packages", // packages_prefix
    "Host",     // host_prefix
    "BIOS",     // bios_prefix
    "CPU",      // cpu_prefix
    "GPU",      // gpu_prefix
    "Memory",   // mem_prefix
    "Public IP",// public_ip_prefix
    "Local IP", // loc_prefix
    "Directory",// pwd_prefix
    "Date",     // date_prefix
    "",         // colors_prefix
    "",         // light_colors_prefix
};

int main(int argc, char **argv) {
    // using a linked list like this is quite horrible, but here we go
    struct Info infos[64] = {
    //  {"Label", fptr, next}
        {NULL, NULL, infos+3},                              // 00
        {config.user_prefix, user, NULL},                   // 01
        {config.hostname_prefix, hostname, NULL},           // 02
        {config.title_prefix, title, infos+23},             // 03
        {config.uptime_prefix, uptime, infos+24},           // 04
        {config.os_prefix, os, infos+6},                    // 05
        {config.kernel_prefix, kernel, infos+7},            // 06
        {config.desktop_prefix, desktop, infos+8},          // 07
        {config.shell_prefix, shell, infos+10},             // 08
        {config.login_shell_prefixix, login_shell, NULL},   // 09
        {config.term_prefix, term, infos+11},               // 10
        {config.packages_prefix, packages, infos+25},       // 11
        {config.host_prefix, host, infos+14},               // 12
        {config.bios_prefix, bios, NULL},                   // 13
        {config.cpu_prefix, cpu, infos+15},                 // 14
        {config.gpu_prefix, gpu, infos+16},                 // 15
        {config.mem_prefix, memory, infos+26},              // 16
        {config.public_ip_prefix, public_ip, NULL},         // 17
        {config.loc_prefix, local_ip, NULL},                // 18
        {config.pwd_prefix, pwd, NULL},                     // 19
        {config.date_prefix, date, NULL},                   // 20
        {config.colors_prefix, colors, infos+22},           // 21
        {config.light_colors_prefix, light_colors, NULL},   // 22
        {config.separator_prefix, separator, infos+4},      // 23
        {config.separator_prefix, separator, infos+5},      // 24
        {config.separator_prefix, separator, infos+12},     // 25
        {config.spacing_prefix, spacing, infos+21},         // 26
    };

    bool user_is_an_idiot = false; // rtfm and stfu

    // are the following command line args used?
    bool asking_help = false;
    int asking_color = 0;
    int asking_bold = 0;
    int asking_logo = 0;
    int asking_align = 0;

    // these store either the default values or the ones defined in the config
    // they are needed to know what is used if no arguments are given (for --help)
    bool default_bold;
    char default_color[8] = "";
    char default_logo[16] = "";

    char *mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    memset(mem, 0, 4096);
    /* this chunk is divided as following (one line is 256B):
     * 0  
     * 1   
     * 2  printed       (saves what should get printed, used for parsing)
     * 3  printed       (saves what should get printed, used for parsing)
     * 4  data          (buffer for every function in info.c)
     * 5  
     * 6  
     * 7  
     * 8  logo          (32 char* to lines (following logo.h))
     * 9  
     * 10 
     * 11 
     * 12 
     * 13 
     * 14 
     * 15 
     */

    // the config that's normally used is ~/.config/albafetch.conf
    char config_file[LOGIN_NAME_MAX + 32] = "";

    char *home = getenv("HOME");
    // I really hope this part will never run
    if(!home) {
        fputs("\e[31m\e[1mERROR\e[0m: $HOME is not set, interrupting!\n", stderr);
        return -1;
    }
    if(!home[0]) {
        fputs("\e[31m\e[1mERROR\e[0m: $HOME is empty, interrupting!\n", stderr);
        return -1;
    }

    // parsing the command args
    for(int i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
            asking_help = true;
        else if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--color"))
            asking_color = i+1;
        else if(!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bold"))
            asking_bold = i+1;
        else if(!strcmp(argv[i], "-l") || !strcmp(argv[i], "--logo"))
            asking_logo = i+1;
        else if(!strcmp(argv[i], "--align") || !strcmp(argv[i], "-a"))
            asking_align = i+1;
        else if(!strcmp(argv[i], "--config")) {
            if(i+1 >= argc) {   // is there such an arg?
                fputs("\e[31m\e[1mERROR\e[0m: --config requires an extra argument!\n", stderr);
                user_is_an_idiot = true;
                continue;
            }
            else if(access(argv[i+1], F_OK)) {  // is it a valid file?
                fprintf(stderr, "\e[31m\e[1mERROR\e[0m: invalid file \"%s\"! Use --help for more info\n", argv[i+1]);
                user_is_an_idiot = true;
                continue;
            }
            strcpy(config_file, argv[i+1]);
            continue;
        }
    }

    if(!config_file[0]) {
        char *config_home = getenv("XDG_CONFIG_HOME");
        if(config_home) { // is XDG_CONFIG_HOME set?
            if(config_home[0]) // is XDG_CONFIG_HOME empty?
                snprintf(config_file, LOGIN_NAME_MAX + 32, "%s/albafetch.conf", config_home);
            else
                snprintf(config_file, LOGIN_NAME_MAX + 32, "%s/.config/albafetch.conf", home);
        }
        else
            snprintf(config_file, LOGIN_NAME_MAX + 32, "%s/.config/albafetch.conf", home);
    }

    parse_config(config_file, &default_bold, default_color, default_logo);

    if(asking_logo) {
        if(asking_logo < argc) {
            bool done = false;

            for(size_t i = 0; i < sizeof(logos)/sizeof(logos[0]); ++i)
                if(!strcmp(logos[i][0], argv[asking_logo])) {
                    config.logo = (char**)logos[i];
                    done = true;
                }

            if(!done) {
                fprintf(stderr, "\e[31m\e[1mERROR\e[0m: invalid logo \"%s\"! Use --help for more info\n", argv[asking_logo]);
                user_is_an_idiot = true;
            }
        } else {
            fputs("\e[31m\e[1mERROR\e[0m: --logo srequires an extra argument!\n", stderr);
            user_is_an_idiot = true;
        }

        strcpy(config.color, config.logo[1]);
    }
    if(!config.logo) {
        #ifdef __APPLE__
            config.logo = (char**)logos[1];
        #else
        # ifdef __ANDROID__
            config.logo = (char**)logos[2];
        # else
            config.logo = (char**)logos[0];
            FILE *fp = fopen("/etc/os-release", "r");

            if(fp) {
                char os_id[48];
                read_after_sequence(fp, "\nID", os_id, 48);
                if(!os_id[0])
                    read_after_sequence(fp, "ID", os_id, 48);
                fclose(fp);

                char *end = strchr(os_id, '\n');
                if(!end)
                    return 2;
                *end = 0;

                for(size_t i = 0; i < sizeof(logos)/sizeof(*logos); ++i)
                    if(!strcmp(logos[i][0], os_id))
                        config.logo = (char**)logos[i];
            }
        # endif // __ANDROID__
        #endif // __APPLE__
        
        strcpy(default_logo, config.logo[0]);
        strcpy(config.color, config.logo[1]);
    }

    if(asking_color) {
        if(asking_color < argc) {
            char *colors[9][2] = {
                {"black", "\e[30m"},
                {"red", "\e[31m"},
                {"green", "\e[32m"},
                {"yellow", "\e[33m"},
                {"blue", "\e[34m"},
                {"purple", "\e[35m"},
                {"cyan", "\e[36m"},
                {"gray", "\e[90m"},
                {"white", "\e[37m"},
            };

            bool done = false;
            for(int j = 0; j < 9; ++j)
                if(!strcmp(argv[asking_color], *colors[j])) {
                    strcpy(config.color, colors[j][1]);
                    done = true;
                }

            if(!done) {
                fprintf(stderr, "\e[31m\e[1mERROR\e[0m: invalid color \"%s\"! Use --help for more info\n", argv[asking_color]);
                user_is_an_idiot = true;
            }
        }
        else {
            fputs("\e[31m\e[1mERROR\e[0m: --color srequires an extra argument!\n", stderr);
            user_is_an_idiot = true;
        }
    }

    if(asking_bold) {
        if(asking_bold < argc) {
            if(!strcmp(argv[asking_bold], "on"))
                config.bold = true;
            else if(!strcmp(argv[asking_bold], "off"))
                config.bold = false;
            else {
                fputs("\e[31m\e[1mERROR\e[0m: --bold should be followed by either \"on\" or \"off\"!\n", stderr);
                user_is_an_idiot = true;
            }
        }
        else {
            fputs("\e[31m\e[1mERROR\e[0m: --bold srequires an extra argument!\n", stderr);
            user_is_an_idiot = true;
        }
    }

    // was it really that hard to type 'albafetch -h'?
    if(user_is_an_idiot) {
        fputs("\e[31m\e[1mFATAL\e[0m: One or multiple errors occured! Use --help for more info\n", stderr);
        return 1;
    }

    if(asking_help) {
        printf("%s%salbafetch\e[0m - a system fetch utility\n",
               config.color, config.bold ? "\e[1m" : "");

        printf("\n%s%sFLAGS\e[0m:\n",
               config.color, config.bold ? "\e[1m" : "");

        printf("\t%s%s-h\e[0m,%s%s --help\e[0m:\t Print this help menu and exit\n",
               config.color, config.bold ? "\e[1m" : "", config.color, config.bold ? "\e[1m" : "");

        printf("\t%s%s-c\e[0m,%s%s --color\e[0m:\t Change the output color (%s%s\e[0m)\n"
               "\t\t\t   [\e[30mblack\e[0m, \e[31mred\e[0m, \e[32mgreen\e[0m, \e[33myellow\e[0m,"
               " \e[34mblue\e[0m, \e[35mpurple\e[0m, \e[36mcyan\e[0m, \e[90mgray\e[0m,"
               " \e[37mwhite\e[0m]\n",
               config.color, config.bold ? "\e[1m" : "", config.color, config.bold ? "\e[1m" : "", default_color[0] ? default_color : config.logo[1], default_color[0] ? "default" : "logo default");

        printf("\t%s%s-b\e[0m,%s%s --bold\e[0m:\t Specifies if bold should be used in colored parts (default: %s\e[0m)\n"
               "\t\t\t   [\e[1mon\e[0m, off]\n",
               config.color, config.bold ? "\e[1m" : "", config.color, config.bold ? "\e[1m" : "", default_bold ? "\e[1mon" : "off");
        
        printf("\t%s%s-l\e[0m,%s%s --logo\e[0m:\t Changes the logo that will be displayed (default: %s)\n"
               "\t\t\t   [android, apple, arch, arch_small, debian, endeavouros, endeavouros, fedora]\n"
               "\t\t\t   [gentoo, linux, linuxmint, manjaro, neon, parrot, pop, ubuntu, windows]\n",
               config.color, config.bold ? "\e[1m" : "", config.color, config.bold ? "\e[1m" : "", default_logo[0] ? default_logo : "OS Default");

        printf("\t%s%s-a\e[0m, %s%s--align\e[0m:\t Alignes the infos if set (default: %s)\n"
               "\t\t\t   [on, off]\n", config.color, config.bold ? "\e[1m" : "", config.color, config.bold ? "\e[1m" : "", config.align ? "on" : "off");

        printf("\t%s%s--config\e[0m:\t Specifies a custom config (default: ~/.config/albafetch.conf)\n"
               "\t\t\t   [path]\n", config.color, config.bold ? "\e[1m" : "");

        printf("\nReport a bug: %s%s\e[4mhttps://github.com/alba4k/albafetch/issues\e[0m\n",
               config.color, config.bold ? "\e[1m" : "");

        return 0;
    }

    if(asking_align) {
        if(asking_align < argc) {
            if(!strcmp(argv[asking_align], "on"))
                config.align = true;
            else if(!strcmp(argv[asking_align], "off"))
                config.align = false;
            else {
                fputs("\e[31m\e[1mERROR\e[0m: --align should be followed by either \"on\" or \"off\"!\n", stderr);
                user_is_an_idiot = true;
            }
        }
        else {
            fputs("\e[31m\e[1mERROR\e[0m: --align srequires an extra argument!\n", stderr);
            user_is_an_idiot = true;
        }
    }

#ifdef _DEBUG
    // Is there a smarter way to do this? Probably, but I'm too lazy
    // Feel free to let me know or just open a PR if you know one <3
    config.os_arch = 1;
    config.de_type = 1;
    config.shell_path = 1;
    config.cpu_brand = 1;
    config.cpu_freq = 1;
    config.cpu_count = 1;
    config.gpu_brand = 1;
    config.mem_perc = 1;
    config.pkg_mgr = 1;
    config.pkg_pacman = 1;
    config.pkg_dpkg = 1;
    config.pkg_rpm = 1;
    config.pkg_flatpak = 1;
    config.pkg_snap = 1;
    config.pkg_pip = 1;
    config.pkg_brew = 1;
    config.loc_localdomain = 1;
    config.loc_docker = 1;

    int (*arr[])(char *) = {
        user,
        hostname,
        uptime,
        os,
        kernel,
        desktop,
        shell,
        login_shell,
        term,
        packages,
        host,
        bios,
        cpu,
        gpu,
        memory,
        public_ip,
        local_ip,
        pwd,
        date,
        colors,
        light_colors
    };

    for(unsigned long i = 0; i < sizeof(arr)/sizeof(arr[0]); ++i) {
        if(!arr[i](mem))
            printf("%s\n", mem);
        else
            printf("%lu returned %d\n", i, arr[i](mem));
    }
    return 0;
#else
    // I am deeply sorry for the code you're about to see - I hope you like spaghettis
    unsigned line = 3;
    char *data = mem + 1024;
    char *printed = mem+512;
    char format[32] = "%s\e[0m%s";
    int escaping = 0;

    if(config.align) {
        int current_len;

        for(struct Info *current = (infos)->next; current; current = current->next) {
            asking_align = 0;
            current_len = strlen(current->label);

            if(current_len > asking_align) {
                asking_align = current_len;
            }
        }

        asking_align += strlen(config.dash);

        snprintf(format, 32, "%%-%ds\e[0m%%s", asking_align);
    }
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    if(w.ws_col == 0) {                             // redirecting stdout results in ws_col = 0 
        ioctl(STDERR_FILENO, TIOCGWINSZ, &w);       // I check whether stderr is fine
        if(w.ws_col == 0) {
            ioctl(STDIN_FILENO, TIOCGWINSZ, &w);    // stdin too
            if(w.ws_col == 0)
                w.ws_col = -1;                      // I just remove the width limit as fallback
        }
    }

    int len_remaining = w.ws_col - strlen(config.logo[2]) - 5;

    for(struct Info *current = infos->next; current; current = current->next) {
        if(current->func == separator) {    // the separators should not get aligned
            if(printed[0]) {                // nothing has been printed
                print_line(config.logo, &line, w.ws_col);
                
                
                for(size_t i = 0, len = 0; (long)len < len_remaining-(int)strlen(config.separator_prefix) && i < strlen(config.separator_prefix); ++i)
                    putc(config.separator_prefix[i], stdout);


                for(size_t i = 0, len = 0; (long)len < len_remaining-(int)strlen(config.separator_prefix) && i < strlen(printed)-4; ++i) {
                    if(!escaping)
                        putc('-', stdout);

                    if(printed[i] != '\e') {
                        // look mom, I just wanted to try to write some branchless code

                        // this line is a bit weird
                        // ++len <=> escaping == 0
                        len += 1-escaping;

                        /* m is found and escaping => escaping = 0
                        * m is found and not escaping => escaping = 0
                        * m is not found and escaping => escaping = 1
                        * m is found and not escaping => escaping = 0
                        */
                        escaping = (printed[i] != 'm') && escaping;
                    }
                    else
                        escaping = 1;
                }
            }
        }
        else if(current->func == spacing) {
            print_line(config.logo, &line, w.ws_col);
        }
        else if(current->func == title) {
            print_line(config.logo, &line, w.ws_col);

            for(size_t i = 0, len = 0; (long)len < len_remaining-(int)strlen(config.title_prefix) && i < strlen(config.title_prefix); ++i)
                putc(config.title_prefix[i], stdout);

            char name[256];
            char host[256];

            char buf[512];

            if(user(name) || hostname(host))
                continue;

            size_t len = strlen(name) + strlen(host) + 5;
            if(len > 511)
                len = 511;

            memset(printed, 'A', len);
            printed[len] = 0;

            snprintf(buf, 512, "%s%s%s%s@%s%s%s", config.title_color ? config.color : "",
                                     config.bold ? "\e[1m" : "",
                                     name,
                                     config.title_color ? "\e[0m" : "",
                                     config.bold ? "\e[1m" : "",
                                     config.title_color ? config.color : "",
                                     host
            );
            for(size_t i = 0, len = 0; (long)len < len_remaining-(int)strlen(config.title_prefix) && i < strlen(buf); ++i) {
                putc(buf[i], stdout);

                if(buf[i] != '\e') {
                    // look mom, I just wanted to try to write some branchless code

                    // this line is a bit weird
                    // ++len <=> escaping == 0
                    len += 1-escaping;

                    /* m is found and escaping => escaping = 0
                    * m is found and not escaping => escaping = 0
                    * m is not found and escaping => escaping = 1
                    * m is found and not escaping => escaping = 0
                    */
                    escaping = (buf[i] != 'm') && escaping;
                }
                else
                    escaping = 1;
            }
        }
        else if(!((current->func)(data))) {
            print_line(config.logo, &line, w.ws_col);

            size_t len = strlen(current->label) + strlen(config.dash);
            char label[len];

            strcpy(label, current->label);
            if(current->label[0] && current->func != colors && current->func != light_colors)
                strcat(label, config.dash);

            snprintf(printed, 512, format, label, data);

            for(size_t i = 0, len = 0; (long)len < len_remaining && i < strlen(printed); ++i) {
                putc(printed[i], stdout);

                if(printed[i] != '\e') {
                    // look mom, I just wanted to try to write some branchless code

                    // this line is a bit weird
                    // ++len <=> escaping == 0
                    len += 1-escaping;

                    /* m is found and escaping => escaping = 0
                    * m is found and not escaping => escaping = 0
                    * m is not found and escaping => escaping = 1
                    * m is found and not escaping => escaping = 0
                    */
                    escaping = (printed[i] != 'm') && escaping;
                }
                else
                    escaping = 1;
            }
        }
    }

    // remaining lines
    while(config.logo[line])
        print_line(config.logo, &line, w.ws_col);
        
    printf("\e[0m\n");
#endif // _DEBUG

    return 0;
}
