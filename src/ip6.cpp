#include <arpa/inet.h>
#include <fstream>
#include <ifaddrs.h>
#include <iostream>
#include <linux/if_link.h>
#include <memory>
#include <net/if.h>
#include <netdb.h>
#include <optional>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

static constexpr int MAXHOST = 1025;
static constexpr int MAXSERV = 32;

std::optional<std::string> default_interface_name();

int main(int argc, char *argv[]) {
  auto host = std::make_unique<char[]>(MAXHOST);
  ifaddrs *ifaddr = nullptr;
  if (-1 == getifaddrs(&ifaddr)) {
    perror("getifaddrs");
    return EXIT_FAILURE;
  }
  auto default_interface = default_interface_name();
  if (!default_interface) {
    std::cerr << "could not find the default interface" << std::endl;
    return EXIT_FAILURE;
  }
  for (ifaddrs *it = ifaddr; it != nullptr; it = it->ifa_next) {
    if (it->ifa_addr == nullptr)
      continue;
    // only bother with the default interface
    if (default_interface.value() != it->ifa_name)
      continue;
    auto family = it->ifa_addr->sa_family;
    if (family & AF_INET6 && !(it->ifa_flags & IFF_LOOPBACK) &&
        (it->ifa_flags & (IFF_UP | IFF_POINTOPOINT))) {
      auto s = getnameinfo(it->ifa_addr, sizeof(sockaddr_in6), host.get(),
                           MAXHOST, nullptr, 0, NI_NUMERICHOST);
      if (0 != s) {
        return EXIT_FAILURE;
      }
      if (strchr(host.get(), ':') != nullptr) {
        // HACK:
        // presence of a colon ":" implies this is an IPv6 address
        puts(host.get());
        break;
      }
    }
  }
  if (ifaddr != nullptr)
    freeifaddrs(ifaddr);
  return EXIT_SUCCESS;
}

std::optional<std::string> default_interface_name() {
  std::ifstream route_file{"/proc/net/route"};
  std::string line{};
  std::getline(route_file, line); // skip first line
  while (std::getline(route_file, line)) {
    std::istringstream ss{line};
    std::string interface, destination;
    std::getline(ss, interface, '\t');
    std::getline(ss, destination, '\t');
    if (destination == "00000000")
      return interface;
  }
  return {};
}
