#include "bootloader.hxx"
#include "constants.hxx"
using namespace beastie;

#include <filesystem>
#include <iostream>
#include <string_view>
#include <format>

#include <getopt.h>
#include <unistd.h>

struct {
    bool debug;
    bool debugAssembly;
    bool pretend;
    bool force;
    std::filesystem::path root;
    unsigned int boot_howto;
} Options;

void version()
{
    std::cout << std::format("{} v{}\n", beastie::progname, beastie::progvers);
}

void usage()
{
    std::cout << std::format("Usage: {} [OPTION]... [root]\n", beastie::progname);
    std::cout << std::format("Directly reboot into FreeBSD\n");
    std::cout << std::format("\n");
    std::cout << std::format(" -h, --help        Print this help.\n");
    std::cout << std::format(" -v, --version     Print the version of {}.\n", beastie::progname);
    std::cout << std::format(" -p, --pretend     Pretend to reboot.\n");
    std::cout << std::format(" -f, --force       Force an immediate boot,\n");
    std::cout << std::format("                   don't call shutdown.\n");
    std::cout << std::format(" -d, --debug       Enable debugging to help spot a failure.\n");
    std::cout << std::format(" -D, --debug-asm   Enable debugging disassembler.\n");
    std::cout << std::format(" -c, --cdrom       Boot in cdrom mode.\n");
    std::cout << std::format(" -s, --serial      Boot in serial mode.\n");
    std::cout << std::format(" -V, --verbose     Boot in verbose mode.\n");
}

int main(int argc, char* argv[])
{
    try {
        int c;
        /* getopt_long stores the option index here. */
        int option_index = 0;

        while (1)
        {
            static struct option long_options[] =
            {
                /* These options set a flag. */
                /*
                 * These options don’t set a flag.
                 * We distinguish them by their indices.
                 */
                {"help",        no_argument,       0, 'h'},
                {"version",     no_argument,       0, 'v'},
                {"pretend",     no_argument,       0, 'p'},
                {"force",       no_argument,       0, 'f'},
                {"debug",       no_argument,       0, 'd'},
                {"debug-asm",   no_argument,       0, 'D'},
                {"cdrom",       no_argument,       0, 'c'},
                {"serial",      no_argument,       0, 's'},
                {"verbose",     no_argument,       0, 'V'},
                {0, 0, 0, 0}
            };

            c = getopt_long (argc, argv, "hvpfdDcsV",
                            long_options, &option_index);

            /* Detect the end of the options. */
            if (c == -1)
                break;

            switch (c)
            {
            case 'h':
                usage();
                return 0;
                break;
            case 'v':
                version();
                return 0;
                break;
            case 'p':
                Options.pretend = true;
                break;
            case 'f':
                Options.force = true;
                break;
            case 'd':
                Options.debug = true;
                break;
            case 'D':
                Options.debug = true;
                Options.debugAssembly = true;
                break;
            case 'c':
                Options.boot_howto |= RB_CDROM;
                break;
            case 's':
                Options.boot_howto |= RB_MULTIPLE;
                Options.boot_howto |= RB_SERIAL;
                break;
            case 'V':
                Options.boot_howto |= RB_VERBOSE;
                break;
            case '?':
                usage();
                return -1;
                break;
            }
        }

        /* positional argument 0: root */
        for(int i = 0; i < argc; ++i) {
            if (std::string_view(argv[i]).empty() ||
                std::string_view(argv[i])[0] == '-')
                continue;
            Options.root = std::filesystem::path(argv[i]);
        }
        if (Options.root.empty()) {
            usage();
            return -1;
        }

        /* check super user permissions */
        auto uid = geteuid();
        if (uid != 0) {
            throw std::runtime_error("This operation requires root privileges. Please run the command as root or use sudo.");
        }

        if (Options.debug)
            std::cout << std::format("boot_howto=0x{:x}\n", Options.boot_howto);

        Bootloader bootloader;
        bootloader.setDebug(Options.debug);
        bootloader.setHowto(Options.boot_howto);
        bootloader.setForce(Options.force);
        bootloader.fontLoad(Options.root/"boot/fonts/12x24.fnt.gz");
        bootloader.fileLoad(Options.root/"boot/kernel/kernel");
        if (Options.pretend == false) {
            bootloader.boot();
        }

    }
    catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
    }

    return 0;
}
