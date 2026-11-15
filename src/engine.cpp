// GamerTCP - Windows 10/11 network optimization toolkit
// Native C++ reimplementation of the GameTCP3 tweak set (BSD 3-Clause).
//
// engine.cpp - tweak tables, registry access, backup/restore, netsh helpers.

#include "engine.h"

#include <shellapi.h>

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <cwctype>

namespace gtc {
namespace {

// ------------------------------------------------------------- key paths

const wchar_t* const kTcpipPath =
    L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
const wchar_t* const kIfacesPath =
    L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
const wchar_t* const kDnsCachePath =
    L"SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters";
const wchar_t* const kServiceProviderPath =
    L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider";
const wchar_t* const kLanmanServerPath =
    L"SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters";
const wchar_t* const kLanmanWorkstationPath =
    L"SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters";
const wchar_t* const kMultimediaPath =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile";
const wchar_t* const kGamesPath =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile\\Tasks\\Games";
const wchar_t* const kMemoryPath =
    L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management";
const wchar_t* const kMsmqPath = L"SOFTWARE\\Microsoft\\MSMQ\\Parameters";
const wchar_t* const kMsmqSecurityPath = L"SOFTWARE\\Microsoft\\MSMQ\\Parameters\\Security";
const wchar_t* const kNetbtPath = L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters";
const wchar_t* const kNetbtIfacesPath =
    L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces";
const wchar_t* const kInternetHkcuPath =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";
const wchar_t* const kInternetHklmPath =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";
const wchar_t* const kPschedPolicyPath = L"SOFTWARE\\Policies\\Microsoft\\Windows\\Psched";
const wchar_t* const kWindowsPolicyPath = L"SOFTWARE\\Policies\\Microsoft\\Windows";
const wchar_t* const kReliabilityPath =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability";
const wchar_t* const kBitsPath = L"SYSTEM\\CurrentControlSet\\Services\\BITS";

// ------------------------------------------------------------- tweak tables

const Tweak kTcpip[] = {
    { Root::LocalMachine, kTcpipPath, L"DefaultTTL", false, 255, nullptr,
      L"Default hop limit for outgoing IPv4 packets" },
    { Root::LocalMachine, kTcpipPath, L"TcpTimedWaitDelay", false, 30, nullptr,
      L"Seconds a closed socket stays in TIME_WAIT" },
    { Root::LocalMachine, kTcpipPath, L"TcpFinWait2Delay", false, 30, nullptr,
      L"Seconds a socket stays in FIN_WAIT_2" },
    { Root::LocalMachine, kTcpipPath, L"TcpMaxDupAcks", false, 2, nullptr,
      L"Duplicate ACKs needed for fast retransmit" },
    { Root::LocalMachine, kTcpipPath, L"TcpMaxSendFree", false, 65535, nullptr,
      L"Maximum send buffers on the free list" },
    { Root::LocalMachine, kTcpipPath, L"StrictTimeWaitSeqCheck", false, 1, nullptr,
      L"Strict TIME_WAIT sequence validation" },
    { Root::LocalMachine, kTcpipPath, L"MaxFreeTcbs", false, 65536, nullptr,
      L"TCP control blocks kept on the free list" },
    { Root::LocalMachine, kTcpipPath, L"MaxUserPort", false, 65534, nullptr,
      L"Highest ephemeral (dynamic) port" },
    { Root::LocalMachine, kTcpipPath, L"MaxHashTableSize", false, 65536, nullptr,
      L"TCP control block hash table size" },
    { Root::LocalMachine, kTcpipPath, L"EnablePMTUBHDetect", false, 1, nullptr,
      L"Detect black-hole routers during PMTU discovery" },
    { Root::LocalMachine, kTcpipPath, L"EnablePMTUDiscovery", false, 1, nullptr,
      L"Discover the path MTU for connections" },
    { Root::LocalMachine, kTcpipPath, L"EnableICMPRedirect", false, 0, nullptr,
      L"Ignore ICMP redirects (security hardening)" },
    { Root::LocalMachine, kTcpipPath, L"SackOpts", false, 1, nullptr,
      L"Selective acknowledgement support" },
    { Root::LocalMachine, kTcpipPath, L"UseDomainNameDevolution", false, 1, nullptr,
      L"Try parent domains during name resolution" },
    { Root::LocalMachine, kTcpipPath, L"EnableIPAutoConfigurationLimits", false, 0, nullptr,
      L"Disable APIPA address limits" },
    { Root::LocalMachine, kTcpipPath, L"SynAttackProtect", false, 1, nullptr,
      L"SYN flood protection" },
    { Root::LocalMachine, kTcpipPath, L"DontAddDefaultGatewayDefault", false, 1, nullptr,
      L"Do not add an implicit default gateway" },
    { Root::LocalMachine, kTcpipPath, L"DeadGWDetectDefault", false, 1, nullptr,
      L"Detect and fail over from dead gateways" },
    { Root::LocalMachine, kTcpipPath, L"EnableConnectionRateLimiting", false, 0, nullptr,
      L"Disable outbound connection rate limiting" },
    { Root::LocalMachine, kTcpipPath, L"TcpNumConnections", false, 16777214, nullptr,
      L"Maximum simultaneous TCP connections" },
    { Root::LocalMachine, kTcpipPath, L"TcpMaxConnectResponseRetransmissions", false, 1, nullptr,
      L"Retransmits of a SYN-ACK before aborting" },
    { Root::LocalMachine, kTcpipPath, L"PPTPTcpMaxDataRetransmissions", false, 2, nullptr,
      L"Data retransmissions for PPTP connections" },
    { Root::LocalMachine, kTcpipPath, L"TcpMaxDataRetransmissions", false, 7, nullptr,
      L"Data retransmissions before a connection drops" },
    { Root::LocalMachine, kTcpipPath, L"TcpMaxConnectRetransmissions", false, 8, nullptr,
      L"SYN retransmissions before a connect fails" },
    { Root::LocalMachine, kTcpipPath, L"TcpCreateAndConnectTcbRateLimitDepth", false, 0, nullptr,
      L"No limit on concurrent TCB creation" },
    { Root::LocalMachine, kTcpipPath, L"DisableTaskOffload", false, 0, nullptr,
      L"Keep TCP/IP task offload enabled" },
    { Root::LocalMachine, kTcpipPath, L"EnableTCPA", false, 1, nullptr,
      L"Enable TCP chimney offload" },
    { Root::LocalMachine, kTcpipPath, L"EnableWsd", false, 0, nullptr,
      L"Disable Web Services Discovery broadcasts" },
    { Root::LocalMachine, kTcpipPath, L"EnableDCA", false, 1, nullptr,
      L"Enable direct cache access" },
    { Root::LocalMachine, kTcpipPath, L"ArpRetryCount", false, 3, nullptr,
      L"ARP resolution retries" },
    { Root::LocalMachine, kTcpipPath, L"ArpTRSingleRoute", false, 1, nullptr,
      L"Use a single route for ARP traffic" },
    { Root::LocalMachine, kTcpipPath, L"IGMPLevel", false, 1, nullptr,
      L"Multicast group membership level" },
    { Root::LocalMachine, kTcpipPath, L"TcpMaxPortsExhausted", false, 65535, nullptr,
      L"Ports kept for half-open connection handling" },
    { Root::LocalMachine, kTcpipPath, L"TcpMaxHalfOpen", false, 200, nullptr,
      L"Maximum half-open (SYN_RECEIVED) connections" },
    { Root::LocalMachine, kTcpipPath, L"TcpMaxHalfOpenRetried", false, 80, nullptr,
      L"Half-open connections that may be retried" },
    { Root::LocalMachine, kTcpipPath, L"FFPControlFlags", false, 1, nullptr,
      L"Fast forward path control flags" },
    { Root::LocalMachine, kTcpipPath, L"TrFunctionalMcastAddress", false, 0, nullptr,
      L"Do not use the functional multicast address" },
    { Root::LocalMachine, kTcpipPath, L"ArpCacheLife", false, 100, nullptr,
      L"Seconds a resolved ARP entry stays valid" },
    { Root::LocalMachine, kTcpipPath, L"ArpCacheMinReferencedLife", false, 900, nullptr,
      L"Minimum lifetime for referenced ARP entries" },
    { Root::LocalMachine, kTcpipPath, L"Tcp1323Opts", false, 0, nullptr,
      L"Window scaling and timestamps (0 = off)" },
    { Root::LocalMachine, kTcpipPath, L"GlobalMaxTcpWindowSize", false, 838040, nullptr,
      L"Global maximum TCP receive window" },
    { Root::LocalMachine, kTcpipPath, L"NumTcbTablePartitions", false, 16, nullptr,
      L"Partitions of the TCB hash table" },
};

const Tweak kInterfaces[] = {
    { Root::LocalMachine, kIfacesPath, L"TCPNoDelay", false, 1, nullptr,
      L"Disable Nagle's algorithm on this interface" },
    { Root::LocalMachine, kIfacesPath, L"TcpAckFrequency", false, 1, nullptr,
      L"Send a delayed ACK for every segment" },
    { Root::LocalMachine, kIfacesPath, L"TcpDelAckTicks", false, 0, nullptr,
      L"Delayed ACK timer in 100 ms ticks" },
    { Root::LocalMachine, kIfacesPath, L"TcpInitialRTT", false, 2, nullptr,
      L"Initial retransmission timeout guess (seconds)" },
    { Root::LocalMachine, kIfacesPath, L"DontAddDefaultGateway", false, 0, nullptr,
      L"Allow this interface to add a default gateway" },
    { Root::LocalMachine, kIfacesPath, L"InterfaceMetric", false, 50, nullptr,
      L"Route metric used for this interface" },
    { Root::LocalMachine, kIfacesPath, L"TcpWindowSize", false, 838040, nullptr,
      L"Per-interface TCP receive window" },
    { Root::LocalMachine, kIfacesPath, L"MTU", false, 1500, nullptr,
      L"Maximum transmission unit" },
};

const Tweak kDnsCache[] = {
    { Root::LocalMachine, kDnsCachePath, L"MaxCacheEntryTtlLimit", false, 64000, nullptr,
      L"Upper bound for positive DNS cache TTL" },
    { Root::LocalMachine, kDnsCachePath, L"MaxSOACacheEntryTtlLimit", false, 301, nullptr,
      L"Upper bound for SOA record cache TTL" },
    { Root::LocalMachine, kDnsCachePath, L"NegativeCacheTime", false, 0, nullptr,
      L"Do not cache negative DNS answers" },
    { Root::LocalMachine, kDnsCachePath, L"NetFailureCacheTime", false, 0, nullptr,
      L"Do not cache network failure results" },
    { Root::LocalMachine, kDnsCachePath, L"NegativeSOACacheTime", false, 0, nullptr,
      L"Do not cache negative SOA answers" },
    { Root::LocalMachine, kDnsCachePath, L"CacheHashTableBucketSize", false, 1, nullptr,
      L"DNS cache hash bucket size" },
    { Root::LocalMachine, kDnsCachePath, L"CacheHashTableSize", false, 384, nullptr,
      L"DNS cache hash table size" },
};

const Tweak kServiceProvider[] = {
    { Root::LocalMachine, kServiceProviderPath, L"DnsPriority", false, 6, nullptr,
      L"Priority of DNS in the name resolution order" },
    { Root::LocalMachine, kServiceProviderPath, L"HostsPriority", false, 5, nullptr,
      L"Priority of the hosts file" },
    { Root::LocalMachine, kServiceProviderPath, L"LocalPriority", false, 4, nullptr,
      L"Priority of the local name cache" },
    { Root::LocalMachine, kServiceProviderPath, L"NetbtPriority", false, 7, nullptr,
      L"Priority of NetBIOS over TCP/IP" },
};

const Tweak kLanmanServer[] = {
    { Root::LocalMachine, kLanmanServerPath, L"Size", false, 3, nullptr,
      L"Server buffer size profile (3 = maximum)" },
    { Root::LocalMachine, kLanmanServerPath, L"IRPStackSize", false, 50, nullptr,
      L"Stack locations reserved for I/O requests" },
    { Root::LocalMachine, kLanmanServerPath, L"SizReqBuf", false, 17424, nullptr,
      L"Size of server request buffers" },
};

const Tweak kLanmanWorkstation[] = {
    { Root::LocalMachine, kLanmanWorkstationPath, L"KeepConn", false, 32000, nullptr,
      L"Seconds an idle SMB session is kept" },
    { Root::LocalMachine, kLanmanWorkstationPath, L"CollectionTime", false, 1, nullptr,
      L"Milliseconds the SMB client waits before sending" },
    { Root::LocalMachine, kLanmanWorkstationPath, L"MaxCollectionCount", false, 1, nullptr,
      L"Characters buffered before an SMB write is sent" },
    { Root::LocalMachine, kLanmanWorkstationPath, L"SizCharBuf", false, 4096, nullptr,
      L"Size of the SMB client character buffer" },
    { Root::LocalMachine, kLanmanWorkstationPath, L"MaxCmds", false, 255, nullptr,
      L"Outstanding SMB commands per session" },
    { Root::LocalMachine, kLanmanWorkstationPath, L"BufFilesDenyWrite", false, 0, nullptr,
      L"Cache read handles opened with deny-write" },
    { Root::LocalMachine, kLanmanWorkstationPath, L"DisableLargeMtu", false, 0, nullptr,
      L"Allow large MTU SMB transfers" },
    { Root::LocalMachine, kLanmanWorkstationPath, L"DisableBandwidthThrottling", false, 1, nullptr,
      L"Disable SMB client bandwidth throttling" },
};

const Tweak kMultimedia[] = {
    { Root::LocalMachine, kMultimediaPath, L"NetworkThrottlingIndex", false, 18309, nullptr,
      L"MMCSS network throttling (18309 = off)" },
    { Root::LocalMachine, kMultimediaPath, L"SystemResponsiveness", false, 2, nullptr,
      L"CPU percentage reserved for background tasks" },
};

const Tweak kGames[] = {
    { Root::LocalMachine, kGamesPath, L"Affinity", false, 0, nullptr,
      L"CPU affinity for the games profile" },
    { Root::LocalMachine, kGamesPath, L"Clock Rate", false, 2710, nullptr,
      L"Timer resolution in 100 ns units" },
    { Root::LocalMachine, kGamesPath, L"GPU Priority", false, 8, nullptr,
      L"GPU scheduling priority" },
    { Root::LocalMachine, kGamesPath, L"Priority", false, 6, nullptr,
      L"Task priority" },
    { Root::LocalMachine, kGamesPath, L"Scheduling Category", true, 0, L"High",
      L"MMCSS scheduling category" },
    { Root::LocalMachine, kGamesPath, L"SFIO Priority", true, 0, L"High",
      L"Storage I/O priority class" },
    { Root::LocalMachine, kGamesPath, L"Background Only", true, 0, L"False",
      L"Whether the games task is background only" },
};

const Tweak kMemory[] = {
    { Root::LocalMachine, kMemoryPath, L"LargeSystemCache", false, 0, nullptr,
      L"Use a small file-system cache (workstation profile)" },
    { Root::LocalMachine, kMemoryPath, L"DisablePagingExecutive", false, 1, nullptr,
      L"Keep kernel code and drivers in physical memory" },
    { Root::LocalMachine, kMemoryPath, L"SecondLevelDataCache", false, 1024, nullptr,
      L"Hint for the L2 cache size" },
    { Root::LocalMachine, kMemoryPath, L"ThirdLevelDataCache", false, 6144, nullptr,
      L"Hint for the L3 cache size" },
    { Root::LocalMachine, kMemoryPath, L"IoPageLockLimit", false, 983040, nullptr,
      L"Bytes of pages that can be locked for I/O" },
};

const Tweak kMsmq[] = {
    { Root::LocalMachine, kMsmqPath, L"TCPNoDelay", false, 1, nullptr,
      L"Disable Nagle's algorithm for MSMQ traffic" },
    { Root::LocalMachine, kMsmqSecurityPath, L"SecureDSCommunication", false, 0, nullptr,
      L"Allow unsecured directory service communication" },
    { Root::LocalMachine, kMsmqSecurityPath, L"LargeSystemCache", false, 0, nullptr,
      L"MSMQ private large system cache flag" },
};

const Tweak kNetbt[] = {
    { Root::LocalMachine, kNetbtPath, L"EnableDns", false, 0, nullptr,
      L"Do not let NetBIOS resolve through DNS" },
    { Root::LocalMachine, kNetbtPath, L"BcastNameQueryCount", false, 3, nullptr,
      L"Broadcast name queries per resolution" },
    { Root::LocalMachine, kNetbtPath, L"EnableLMHOSTS", false, 1, nullptr,
      L"Use the LMHOSTS file" },
    { Root::LocalMachine, kNetbtPath, L"NameSrvQueryCount", false, 3, nullptr,
      L"WINS name server queries per resolution" },
    { Root::LocalMachine, kNetbtPath, L"UseNewSmb", false, 1, nullptr,
      L"Use the newer SMB dialect for NetBIOS sessions" },
    { Root::LocalMachine, kNetbtPath, L"EnableProxy", false, 0, nullptr,
      L"Do not act as a WINS proxy" },
    { Root::LocalMachine, kNetbtPath, L"EnableProxyRegCheck", false, 0, nullptr,
      L"Do not register as a WINS proxy" },
};

const Tweak kNetbtIfaces[] = {
    { Root::LocalMachine, kNetbtIfacesPath, L"NetbiosOptions", false, 1, nullptr,
      L"NetBIOS over TCP/IP policy (1 = enabled)" },
};

const Tweak kInternet[] = {
    { Root::CurrentUser, kInternetHkcuPath, L"MaxConnectionsPerServer", false, 69, nullptr,
      L"WinINET HTTP/1.1 connections per server" },
    { Root::CurrentUser, kInternetHkcuPath, L"MaxConnectionsPer1_0Server", false, 69, nullptr,
      L"WinINET HTTP/1.0 connections per server" },
    { Root::LocalMachine, kInternetHklmPath, L"MaxConnectionsPerServer", false, 16, nullptr,
      L"Machine-wide HTTP/1.1 connections per server" },
    { Root::LocalMachine, kInternetHklmPath, L"MaxConnectionsPer1_0Server", false, 16, nullptr,
      L"Machine-wide HTTP/1.0 connections per server" },
    { Root::LocalMachine, kInternetHklmPath, L"SmoothScroll", false, 1, nullptr,
      L"Enable smooth scrolling in WinINET hosts" },
};

const Tweak kPolicies[] = {
    { Root::LocalMachine, kPschedPolicyPath, L"NonBestEffortLimit", false, 0, nullptr,
      L"QoS reserved bandwidth percentage (0 = none reserved)" },
    { Root::LocalMachine, kWindowsPolicyPath, L"NonBestEffortLimit", false, 0, nullptr,
      L"Legacy QoS reserved bandwidth percentage" },
};

const Tweak kMisc[] = {
    { Root::LocalMachine, kReliabilityPath, L"TimeStampInterval", false, 0, nullptr,
      L"Disable reliability monitor timestamping" },
    { Root::LocalMachine, kBitsPath, L"Start", false, 3, nullptr,
      L"BITS service start mode (3 = manual)" },
};

const Section kSections[] = {
    { L"tcpip", L"TCP/IP Parameters",
      L"Core transport parameters: TTL, ephemeral ports, retransmissions, window size, "
      L"ARP caching and SYN flood protection.",
      SectionMode::Fixed, kTcpip, static_cast<int>(sizeof(kTcpip) / sizeof(kTcpip[0])), true },
    { L"interfaces", L"Network Interface Settings",
      L"Applied to every network interface registry key: Nagle's algorithm, delayed ACK, "
      L"initial RTT, route metric, window size and MTU.",
      SectionMode::RecurseSubkeys, kInterfaces,
      static_cast<int>(sizeof(kInterfaces) / sizeof(kInterfaces[0])), true },
    { L"dnscache", L"DNS Cache Settings",
      L"Resolver cache sizes and negative-answer caching.",
      SectionMode::Fixed, kDnsCache, static_cast<int>(sizeof(kDnsCache) / sizeof(kDnsCache[0])),
      true },
    { L"serviceprovider", L"Service Provider Settings",
      L"Order in which name resolution providers are consulted.",
      SectionMode::Fixed, kServiceProvider,
      static_cast<int>(sizeof(kServiceProvider) / sizeof(kServiceProvider[0])), true },
    { L"lanmanserver", L"LanmanServer Settings",
      L"SMB server buffer and work-item sizing.",
      SectionMode::Fixed, kLanmanServer,
      static_cast<int>(sizeof(kLanmanServer) / sizeof(kLanmanServer[0])), true },
    { L"lanmanworkstation", L"LanmanWorkstation Settings",
      L"SMB client buffers, command limits and bandwidth throttling.",
      SectionMode::Fixed, kLanmanWorkstation,
      static_cast<int>(sizeof(kLanmanWorkstation) / sizeof(kLanmanWorkstation[0])), true },
    { L"multimedia", L"Multimedia Settings",
      L"MMCSS network throttling index and system responsiveness.",
      SectionMode::Fixed, kMultimedia, static_cast<int>(sizeof(kMultimedia) / sizeof(kMultimedia[0])),
      true },
    { L"games", L"Games Task Profile",
      L"Games task scheduling: CPU/GPU priority, clock rate and MMCSS category.",
      SectionMode::Fixed, kGames, static_cast<int>(sizeof(kGames) / sizeof(kGames[0])), true },
    { L"memory", L"Memory Management",
      L"Kernel paging behaviour and file-system cache sizing.",
      SectionMode::Fixed, kMemory, static_cast<int>(sizeof(kMemory) / sizeof(kMemory[0])), true },
    { L"msmq", L"MSMQ Settings",
      L"Message queuing transport delay and directory service communication.",
      SectionMode::Fixed, kMsmq, static_cast<int>(sizeof(kMsmq) / sizeof(kMsmq[0])), true },
    { L"netbt", L"NetBT Settings",
      L"NetBIOS over TCP/IP name resolution, proxying and per-interface NetbiosOptions.",
      SectionMode::Fixed, kNetbt, static_cast<int>(sizeof(kNetbt) / sizeof(kNetbt[0])), true },
    { L"netbt-interfaces", L"NetBT Interface Settings",
      L"NetbiosOptions applied to every NetBT interface key.",
      SectionMode::NetbtInterfaces, kNetbtIfaces,
      static_cast<int>(sizeof(kNetbtIfaces) / sizeof(kNetbtIfaces[0])), true },
    { L"internet", L"Internet Settings",
      L"WinINET connection limits and scrolling (current user and machine wide).",
      SectionMode::Fixed, kInternet, static_cast<int>(sizeof(kInternet) / sizeof(kInternet[0])),
      true },
    { L"policies", L"Policy Settings",
      L"QoS policy limiting reserved bandwidth.",
      SectionMode::Fixed, kPolicies, static_cast<int>(sizeof(kPolicies) / sizeof(kPolicies[0])),
      true },
    { L"misc", L"Miscellaneous Settings",
      L"Reliability monitor timestamping and the BITS service start mode.",
      SectionMode::Fixed, kMisc, static_cast<int>(sizeof(kMisc) / sizeof(kMisc[0])), true },
};

const std::vector<std::wstring> kResetCommands = {
    L"netsh int tcp reset",
    L"netsh int ipv4 reset",
    L"netsh int ipv6 reset",
    L"netsh int ip reset",
    L"netsh winsock reset catalog",
    L"netsh int ip delete neighbors",
    L"netsh Int tcp set global nonsackrttresiliency=disabled",
    L"netsh Int tcp set global chimney=disabled",
    L"netsh Int tcp set global netdma=enabled",
    L"netsh interface ip delete arpcache",
    L"netsh Int tcp set global rss=enabled",
    L"netsh Int tcp set supplemental congestionprovider=ctcp",
    L"netsh Int tcp set global initialRto=2000",
    L"netsh Int tcp set global dca=enabled",
    L"netsh interface tcp set global ecncapability=disabled",
    L"netsh int tcp set global autotuninglevel=normal",
    L"netsh int tcp set global rsc=disabled",
    L"netsh Int ipv4 set glob defaultcurhoplimit=255",
    L"netsh Int tcp set global maxsynretransmissions=2",
    L"netsh Int ipv6 set glob defaultcurhoplimit=255",
    L"netsh int tcp set heuristics disabled",
    L"netsh int tcp set global timestamps=disabled",
    L"netsh int tcp set global fastopen=disabled",
};

const TeredoOption kTeredoOptions[] = {
    { L"Disable Teredo (recommended for gaming)", L"disabled",
      L"Turns the Teredo tunnel adapter off." },
    { L"Enable Teredo with default settings", L"type=default",
      L"Restores the Windows default Teredo configuration." },
    { L"Enable Teredo with enterprise client", L"enterpriseclient",
      L"Uses an enterprise-managed Teredo relay/server." },
};

// ------------------------------------------------------------- internals

struct Target {
    Root         root;
    std::wstring path;
    std::wstring sub;
    const Tweak* tweak;
};

void EnumSubkeys(Root root, const std::wstring& path, bool recursive, bool requireValues,
                 const std::function<void(const std::wstring&)>& cb, int* errors) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HiveOf(root), path.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key) !=
        ERROR_SUCCESS) {
        if (errors) ++*errors;
        return;
    }

    for (DWORD index = 0;; ++index) {
        wchar_t name[256];
        DWORD length = static_cast<DWORD>(sizeof(name) / sizeof(name[0]));
        LONG result = RegEnumKeyExW(key, index, name, &length, nullptr, nullptr, nullptr, nullptr);
        if (result == ERROR_NO_MORE_ITEMS) break;
        if (result != ERROR_SUCCESS) {
            if (errors) ++*errors;
            break;
        }

        const std::wstring child = path + L"\\" + name;
        DWORD values = 0;
        DWORD subkeys = 0;
        HKEY childKey = nullptr;
        if (RegOpenKeyExW(HiveOf(root), child.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &childKey) ==
            ERROR_SUCCESS) {
            RegQueryInfoKeyW(childKey, nullptr, nullptr, nullptr, &subkeys, nullptr, nullptr,
                             &values, nullptr, nullptr, nullptr, nullptr);
            RegCloseKey(childKey);

            if (!requireValues || values > 0) cb(child);
            if (recursive && subkeys > 0)
                EnumSubkeys(root, child, recursive, requireValues, cb, errors);
        }
    }

    RegCloseKey(key);
}

void CollectTargets(const Section& section, std::vector<Target>& out, int* errors,
                    const TweakFilter& filter) {
    if (section.mode == SectionMode::Fixed) {
        for (int i = 0; i < section.count; ++i) {
            const Tweak& tweak = section.tweaks[i];
            if (filter && !filter(section, tweak)) continue;
            out.push_back(Target{tweak.root, tweak.path, std::wstring(), &tweak});
        }
        return;
    }

    const std::wstring base = section.tweaks[0].path;
    const bool recursive = section.mode == SectionMode::RecurseSubkeys;
    const bool requireValues = section.mode == SectionMode::RecurseSubkeys;

    EnumSubkeys(
        section.tweaks[0].root, base, recursive, requireValues,
        [&](const std::wstring& full) {
            std::wstring sub = full.substr(base.size());
            if (!sub.empty() && sub[0] == L'\\') sub.erase(0, 1);
            for (int i = 0; i < section.count; ++i) {
                const Tweak& tweak = section.tweaks[i];
                if (filter && !filter(section, tweak)) continue;
                out.push_back(Target{tweak.root, full, sub, &tweak});
            }
        },
        errors);
}

bool SectionHasSelectedTweaks(const Section& section, const TweakFilter& filter) {
    if (!filter) return true;
    for (int i = 0; i < section.count; ++i) {
        if (filter(section, section.tweaks[i])) return true;
    }
    return false;
}

bool Matches(const OldValue& old, const Tweak& tweak) {
    if (!old.exists) return false;
    if (tweak.isString) {
        return (old.type == REG_SZ || old.type == REG_EXPAND_SZ) && old.text == tweak.sz;
    }
    return old.type == REG_DWORD && old.dw == tweak.dw;
}

std::wstring TargetText(const Tweak& tweak) {
    if (tweak.isString) return tweak.sz ? tweak.sz : L"";
    return std::to_wstring(tweak.dw);
}

LONG WriteTweak(const Tweak& tweak, const std::wstring& path) {
    if (tweak.isString) return WriteString(tweak.root, path, tweak.name, tweak.sz);
    return WriteDword(tweak.root, path, tweak.name, tweak.dw);
}

Entry MakeEntry(const Section& section, const Target& target, const OldValue& old) {
    Entry entry;
    entry.section = section.name;
    entry.sub = target.sub;
    entry.path = DisplayPath(target.root, target.path);
    entry.name = target.tweak->name;
    entry.oldData = old.Display();
    entry.newData = TargetText(*target.tweak);
    return entry;
}

Entry ApplyOne(const Section& section, const Target& target, bool dryRun) {
    const OldValue old = ReadValue(target.root, target.path, target.tweak->name);
    Entry entry = MakeEntry(section, target, old);

    if (Matches(old, *target.tweak)) {
        entry.state = State::Unchanged;
        return entry;
    }
    if (dryRun) {
        entry.state = State::Changed;
        return entry;
    }

    const LONG result = WriteTweak(*target.tweak, target.path);
    if (result != ERROR_SUCCESS) {
        entry.state = State::Failed;
        entry.error = FormatWin32Error(result);
    } else {
        entry.state = State::Changed;
    }
    return entry;
}

std::wstring HexEncode(const std::wstring& text) {
    static const wchar_t* digits = L"0123456789ABCDEF";
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(text.data());
    const size_t count = text.size() * sizeof(wchar_t);
    std::wstring out;
    out.reserve(count * 2);
    for (size_t i = 0; i < count; ++i) {
        out.push_back(digits[bytes[i] >> 4]);
        out.push_back(digits[bytes[i] & 0x0F]);
    }
    return out;
}

int HexDigit(wchar_t c) {
    if (c >= L'0' && c <= L'9') return c - L'0';
    if (c >= L'a' && c <= L'f') return c - L'a' + 10;
    if (c >= L'A' && c <= L'F') return c - L'A' + 10;
    return -1;
}

bool HexDecode(const std::wstring& hex, std::wstring* text) {
    if (hex.size() % 2 != 0) return false;
    std::string bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        const int hi = HexDigit(hex[i]);
        const int lo = HexDigit(hex[i + 1]);
        if (hi < 0 || lo < 0) return false;
        bytes.push_back(static_cast<char>((hi << 4) | lo));
    }
    if (bytes.size() % sizeof(wchar_t) != 0) return false;
    text->assign(bytes.size() / sizeof(wchar_t), L'\0');
    if (!bytes.empty()) std::memcpy(&(*text)[0], bytes.data(), bytes.size());
    return true;
}

std::wstring Timestamp() {
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%04u-%02u-%02u %02u:%02u:%02u",
                  now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    return buffer;
}

std::wstring FileStamp() {
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%04u%02u%02u-%02u%02u%02u",
                  now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    return buffer;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) return std::string();
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), &out[0], size,
                        nullptr, nullptr);
    return out;
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) return std::wstring();
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                         nullptr, 0);
    std::wstring out(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), &out[0], size);
    return out;
}

std::wstring DecodeConsoleBytes(const std::string& bytes) {
    const UINT pages[] = {CP_OEMCP, CP_ACP, CP_UTF8};
    for (UINT page : pages) {
        if (bytes.empty()) return std::wstring();
        const int size = MultiByteToWideChar(page, 0, bytes.data(), static_cast<int>(bytes.size()),
                                             nullptr, 0);
        if (size <= 0) continue;
        std::wstring out(static_cast<size_t>(size), L'\0');
        const int written = MultiByteToWideChar(page, 0, bytes.data(),
                                                static_cast<int>(bytes.size()), &out[0], size);
        if (written > 0) {
            out.resize(static_cast<size_t>(written));
            return out;
        }
    }
    return std::wstring();
}

std::wstring BackupData(const OldValue& value) {
    if (!value.exists) return L"MISSING|";
    if (value.type == REG_DWORD) {
        wchar_t buffer[16];
        std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"DWORD|%08X", value.dw);
        return buffer;
    }
    if (value.type == REG_SZ || value.type == REG_EXPAND_SZ)
        return L"SZ|" + HexEncode(value.text);
    return L"MISSING|";
}

bool WriteAllBytes(const std::wstring& file, const std::string& bytes) {
    HANDLE handle = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const BOOL ok = WriteFile(handle, bytes.data(), static_cast<DWORD>(bytes.size()), &written,
                              nullptr);
    CloseHandle(handle);
    return ok != FALSE && written == bytes.size();
}

}  // namespace

// ------------------------------------------------------------- metadata

const wchar_t* VersionString() { return L"4.2.0"; }

const wchar_t* CopyrightString() {
    return L"BSD 3-Clause. Tweak set derived from GameTCP3 "
           L"(c) 2022 Charles McDonald.";
}

const wchar_t* RootTag(Root root) {
    return root == Root::CurrentUser ? L"HKCU" : L"HKLM";
}

HKEY HiveOf(Root root) {
    return root == Root::CurrentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
}

std::wstring DisplayPath(Root root, const std::wstring& path) {
    return std::wstring(RootTag(root)) + L"\\" + path;
}

const std::vector<Section>& Sections() {
    static const std::vector<Section> all(kSections,
                                          kSections + sizeof(kSections) / sizeof(kSections[0]));
    return all;
}

const Section* FindSection(const std::wstring& idOrName) {
    const std::wstring needle = Lower(Trim(idOrName));
    if (needle.empty()) return nullptr;
    for (const Section& section : Sections()) {
        if (Lower(section.id) == needle || Lower(section.name) == needle) return &section;
    }
    // Fall back to a unique prefix of the id.
    const Section* found = nullptr;
    for (const Section& section : Sections()) {
        const std::wstring id = Lower(section.id);
        if (id.rfind(needle, 0) == 0) {
            if (found) return nullptr;
            found = &section;
        }
    }
    return found;
}

std::wstring Lower(const std::wstring& text) {
    std::wstring out = text;
    for (wchar_t& c : out) c = static_cast<wchar_t>(std::towlower(c));
    return out;
}

std::vector<std::wstring> SplitList(const std::wstring& text, wchar_t sep) {
    std::vector<std::wstring> parts;
    std::wstring current;
    for (wchar_t c : text) {
        if (c == sep) {
            parts.push_back(Trim(current));
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    parts.push_back(Trim(current));
    return parts;
}

std::wstring Trim(const std::wstring& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < end && std::iswspace(text[start])) ++start;
    while (end > start && std::iswspace(text[end - 1])) --end;
    return text.substr(start, end - start);
}

std::wstring FormatWin32Error(LONG code) {
    wchar_t* message = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, static_cast<DWORD>(code), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<wchar_t*>(&message), 0, nullptr);
    std::wstring text;
    if (length && message) {
        text.assign(message, length);
        text = Trim(text);
    }
    if (message) LocalFree(message);
    if (text.empty()) text = L"error " + std::to_wstring(code);
    return text;
}

// ------------------------------------------------------------- registry

std::wstring OldValue::Display() const {
    if (!exists) return L"(not set)";
    if (type == REG_DWORD) return std::to_wstring(dw);
    if (type == REG_SZ || type == REG_EXPAND_SZ) return text.empty() ? L"(empty)" : text;
    if (text.empty()) return L"(binary)";
    return text;
}

OldValue ReadValue(Root root, const std::wstring& path, const std::wstring& name) {
    OldValue value;
    HKEY key = nullptr;
    if (RegOpenKeyExW(HiveOf(root), path.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key) !=
        ERROR_SUCCESS) {
        return value;
    }

    DWORD type = 0;
    DWORD size = 0;
    LONG result = RegQueryValueExW(key, name.c_str(), nullptr, &type, nullptr, &size);
    if (result == ERROR_SUCCESS && size <= 64 * 1024) {
        std::vector<BYTE> data(size ? size : 1);
        DWORD read = size;
        result = RegQueryValueExW(key, name.c_str(), nullptr, &type, data.data(), &read);
        if (result == ERROR_SUCCESS) {
            value.exists = true;
            value.type = type;
            if (type == REG_DWORD && read >= sizeof(DWORD)) {
                std::memcpy(&value.dw, data.data(), sizeof(DWORD));
            } else if ((type == REG_SZ || type == REG_EXPAND_SZ) && read >= sizeof(wchar_t)) {
                value.text.assign(reinterpret_cast<const wchar_t*>(data.data()),
                                  read / sizeof(wchar_t));
                while (!value.text.empty() && value.text.back() == L'\0') value.text.pop_back();
            } else {
                std::wstring raw(read / sizeof(wchar_t), L'\0');
                if (!raw.empty()) std::memcpy(&raw[0], data.data(), raw.size() * sizeof(wchar_t));
                value.text = HexEncode(raw);
            }
        }
    }

    RegCloseKey(key);
    return value;
}

LONG WriteDword(Root root, const std::wstring& path, const std::wstring& name, DWORD value) {
    HKEY key = nullptr;
    LONG result = RegCreateKeyExW(HiveOf(root), path.c_str(), 0, nullptr,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_WOW64_64KEY, nullptr, &key,
                                  nullptr);
    if (result != ERROR_SUCCESS) return result;
    result = RegSetValueExW(key, name.c_str(), 0, REG_DWORD,
                            reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(key);
    return result;
}

LONG WriteString(Root root, const std::wstring& path, const std::wstring& name,
                 const std::wstring& value) {
    HKEY key = nullptr;
    LONG result = RegCreateKeyExW(HiveOf(root), path.c_str(), 0, nullptr,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_WOW64_64KEY, nullptr, &key,
                                  nullptr);
    if (result != ERROR_SUCCESS) return result;
    const std::wstring terminated = value;
    result = RegSetValueExW(
        key, name.c_str(), 0, REG_SZ,
        reinterpret_cast<const BYTE*>(terminated.c_str()),
        static_cast<DWORD>((terminated.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return result;
}

LONG DeleteValue(Root root, const std::wstring& path, const std::wstring& name) {
    HKEY key = nullptr;
    LONG result = RegOpenKeyExW(HiveOf(root), path.c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY,
                                &key);
    if (result != ERROR_SUCCESS) return result;
    result = RegDeleteValueW(key, name.c_str());
    RegCloseKey(key);
    return result;
}

// ------------------------------------------------------------- apply/scan

void Summary::Add(const Entry& entry) {
    switch (entry.state) {
        case State::Changed:
        case State::Deleted:
            ++changed;
            break;
        case State::Failed:
            ++failed;
            break;
        default:
            ++unchanged;
            break;
    }
}

Summary ApplySections(const std::vector<const Section*>& sections, const ApplyOptions& options,
                      const EntrySink& sink) {
    Summary summary;
    for (const Section* section : sections) {
        if (!section) continue;
        if (!SectionHasSelectedTweaks(*section, options.filter)) continue;

        std::vector<Target> targets;
        int errors = 0;
        CollectTargets(*section, targets, &errors, options.filter);

        if (errors > 0 && targets.empty()) {
            Entry entry;
            entry.section = section->name;
            entry.state = State::Failed;
            entry.error = FormatWin32Error(ERROR_FILE_NOT_FOUND) + L" (no readable subkeys)";
            summary.Add(entry);
            if (sink) sink(entry);
            continue;
        }

        for (const Target& target : targets) {
            const Entry entry = ApplyOne(*section, target, options.dryRun);
            summary.Add(entry);
            if (sink) sink(entry);
        }
    }
    return summary;
}

Summary ScanSections(const std::vector<const Section*>& sections, std::vector<ScanRow>& rows,
                     const TweakFilter& filter) {
    Summary summary;
    for (const Section* section : sections) {
        if (!section) continue;
        if (!SectionHasSelectedTweaks(*section, filter)) continue;

        std::vector<Target> targets;
        int errors = 0;
        CollectTargets(*section, targets, &errors, filter);

        if (errors > 0 && targets.empty()) {
            ScanRow row;
            row.section = section->name;
            row.name = L"(section)";
            row.current = L"unavailable";
            row.failed = true;
            rows.push_back(row);
            ++summary.failed;
            continue;
        }

        for (const Target& target : targets) {
            const OldValue value = ReadValue(target.root, target.path, target.tweak->name);
            ScanRow row;
            row.section = section->name;
            row.tweak = target.tweak;
            row.sub = target.sub;
            row.name = target.tweak->name;
            row.current = value.Display();
            row.target = TargetText(*target.tweak);
            row.exists = value.exists;
            row.matches = Matches(value, *target.tweak);
            rows.push_back(row);
            if (row.matches)
                ++summary.unchanged;
            else
                ++summary.changed;
        }
    }
    return summary;
}

// ------------------------------------------------------------- backups

std::wstring DefaultBackupDirectory() {
    wchar_t buffer[MAX_PATH];
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer,
                                           static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])));
    std::wstring base;
    if (length > 0 && length < sizeof(buffer) / sizeof(buffer[0])) {
        base.assign(buffer, length);
    } else {
        base = L".";
    }
    return base + L"\\GamerTCP";
}

std::wstring DefaultBackupPath() {
    return DefaultBackupDirectory() + L"\\backup-" + FileStamp() + L".txt";
}

bool BackupSections(const std::vector<const Section*>& sections, const std::wstring& file,
                    std::wstring* error, const TweakFilter& filter) {
    // Make sure the destination directory exists (one level is enough here).
    const size_t slash = file.find_last_of(L'\\');
    if (slash != std::wstring::npos) {
        const std::wstring directory = file.substr(0, slash);
        if (!directory.empty()) CreateDirectoryW(directory.c_str(), nullptr);
    }

    std::wstring text;
    text += L"# GamerTCP backup v1\r\n";
    text += L"# created " + Timestamp() + L"\r\n";
    text += L"# format: HIVE|KEY_PATH|VALUE_NAME|TYPE|DATA\r\n";

    int failures = 0;
    for (const Section* section : sections) {
        if (!section) continue;
        if (!SectionHasSelectedTweaks(*section, filter)) continue;

        std::vector<Target> targets;
        int errors = 0;
        CollectTargets(*section, targets, &errors, filter);
        if (errors > 0) ++failures;

        text += L"# section " + std::wstring(section->id) + L"\r\n";
        for (const Target& target : targets) {
            const OldValue value = ReadValue(target.root, target.path, target.tweak->name);
            text += RootTag(target.root);
            text += L"|";
            text += target.path;
            text += L"|";
            text += target.tweak->name;
            text += L"|";
            text += BackupData(value);
            text += L"\r\n";
        }
    }

    std::string bytes;
    bytes.reserve(text.size() * 2 + 3);
    bytes.push_back('\xEF');
    bytes.push_back('\xBB');
    bytes.push_back('\xBF');
    bytes += WideToUtf8(text);

    if (!WriteAllBytes(file, bytes)) {
        if (error) *error = FormatWin32Error(static_cast<LONG>(GetLastError()));
        return false;
    }
    if (error) {
        error->clear();
        if (failures > 0) *error = L"some subkeys could not be enumerated";
    }
    return true;
}

bool ReadBackupFile(const std::wstring& file, std::vector<BackupRow>& rows, std::wstring* error) {
    rows.clear();
    HANDLE handle = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        if (error) *error = FormatWin32Error(static_cast<LONG>(GetLastError()));
        return false;
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(handle, &size) || size.QuadPart > 32 * 1024 * 1024) {
        CloseHandle(handle);
        if (error) *error = L"backup file is too large";
        return false;
    }

    std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    if (!bytes.empty()) {
        if (!ReadFile(handle, &bytes[0], static_cast<DWORD>(bytes.size()), &read, nullptr)) {
            CloseHandle(handle);
            if (error) *error = FormatWin32Error(static_cast<LONG>(GetLastError()));
            return false;
        }
        bytes.resize(read);
    }
    CloseHandle(handle);

    if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF) {
        bytes.erase(0, 3);
    }

    const std::wstring text = Utf8ToWide(bytes);
    for (const std::wstring& rawLine : SplitList(text, L'\n')) {
        const std::wstring line = Trim(rawLine);
        if (line.empty() || line[0] == L'#') continue;

        // HIVE|PATH|NAME|TYPE|DATA - split on the first four pipes only.
        std::vector<std::wstring> fields;
        std::wstring current;
        for (wchar_t c : line) {
            if (c == L'|' && fields.size() < 4) {
                fields.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
        fields.push_back(current);
        if (fields.size() != 5) continue;

        BackupRow row;
        if (Lower(fields[0]) == L"hklm")
            row.root = Root::LocalMachine;
        else if (Lower(fields[0]) == L"hkcu")
            row.root = Root::CurrentUser;
        else
            continue;

        row.path = fields[1];
        row.name = fields[2];
        const std::wstring type = Lower(fields[3]);
        const std::wstring data = fields[4];

        if (type == L"missing") {
            row.kind = BackupRow::Kind::Missing;
        } else if (type == L"dword") {
            row.kind = BackupRow::Kind::Dword;
            row.dw = static_cast<DWORD>(std::wcstoul(data.c_str(), nullptr, 16));
        } else if (type == L"sz") {
            row.kind = BackupRow::Kind::String;
            if (!HexDecode(data, &row.sz)) continue;
        } else {
            continue;
        }
        rows.push_back(row);
    }

    if (rows.empty()) {
        if (error) *error = L"no usable entries found (is this a GamerTCP backup?)";
        return false;
    }
    if (error) error->clear();
    return true;
}

bool RestoreBackup(const std::wstring& file, const EntrySink& sink, Summary* summary,
                   std::wstring* error) {
    std::vector<BackupRow> rows;
    if (!ReadBackupFile(file, rows, error)) return false;

    Summary local;
    for (const BackupRow& row : rows) {
        Entry entry;
        entry.section = L"restore";
        entry.path = DisplayPath(row.root, row.path);
        entry.name = row.name;

        const OldValue current = ReadValue(row.root, row.path, row.name);
        entry.oldData = current.Display();

        if (row.kind == BackupRow::Kind::Missing) {
            entry.newData = L"(not set)";
            if (!current.exists) {
                entry.state = State::Unchanged;
            } else {
                const LONG result = DeleteValue(row.root, row.path, row.name);
                if (result == ERROR_SUCCESS) {
                    entry.state = State::Deleted;
                } else {
                    entry.state = State::Failed;
                    entry.error = FormatWin32Error(result);
                }
            }
        } else if (row.kind == BackupRow::Kind::Dword) {
            entry.newData = std::to_wstring(row.dw);
            if (current.exists && current.type == REG_DWORD && current.dw == row.dw) {
                entry.state = State::Unchanged;
            } else {
                const LONG result = WriteDword(row.root, row.path, row.name, row.dw);
                entry.state = result == ERROR_SUCCESS ? State::Changed : State::Failed;
                if (result != ERROR_SUCCESS) entry.error = FormatWin32Error(result);
            }
        } else {
            entry.newData = row.sz;
            if (current.exists && (current.type == REG_SZ || current.type == REG_EXPAND_SZ) &&
                current.text == row.sz) {
                entry.state = State::Unchanged;
            } else {
                const LONG result = WriteString(row.root, row.path, row.name, row.sz);
                entry.state = result == ERROR_SUCCESS ? State::Changed : State::Failed;
                if (result != ERROR_SUCCESS) entry.error = FormatWin32Error(result);
            }
        }

        local.Add(entry);
        if (sink) sink(entry);
    }

    if (summary) *summary = local;
    return true;
}

// ------------------------------------------------------------- commands

const std::vector<std::wstring>& ResetStackCommands() { return kResetCommands; }

bool RunAndCapture(const std::wstring& commandLine, std::wstring* output, DWORD* exitCode) {
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &attributes, 0)) return false;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdOutput = writePipe;
    startup.hStdError = writePipe;
    startup.hStdInput = INVALID_HANDLE_VALUE;

    PROCESS_INFORMATION process{};
    std::wstring mutableCommand = commandLine;
    const BOOL started = CreateProcessW(nullptr, &mutableCommand[0], nullptr, nullptr, TRUE,
                                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(writePipe);
    if (!started) {
        CloseHandle(readPipe);
        return false;
    }

    std::string bytes;
    char buffer[4096];
    DWORD got = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &got, nullptr) && got > 0) {
        bytes.append(buffer, got);
    }

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    CloseHandle(readPipe);

    if (output) *output = Trim(DecodeConsoleBytes(bytes));
    if (exitCode) *exitCode = code;
    return true;
}

CommandSummary RunCommands(const std::vector<std::wstring>& commands, bool dryRun,
                           const CommandSink& sink) {
    CommandSummary summary;
    for (const std::wstring& command : commands) {
        CommandResult result;
        result.command = command;

        if (dryRun) {
            result.ok = true;
            result.output = L"(dry run)";
            ++summary.succeeded;
        } else {
            std::wstring output;
            DWORD code = 0;
            const std::wstring line = L"cmd.exe /c " + command;
            if (RunAndCapture(line, &output, &code)) {
                result.exitCode = code;
                result.output = output;
                result.ok = (code == 0);
            } else {
                result.exitCode = static_cast<DWORD>(-1);
                result.output = L"could not start cmd.exe: " +
                                FormatWin32Error(static_cast<LONG>(GetLastError()));
                result.ok = false;
            }
            if (result.ok)
                ++summary.succeeded;
            else
                ++summary.failed;
        }

        if (sink) sink(result);
    }
    return summary;
}

// ------------------------------------------------------------- teredo

const TeredoOption* TeredoOptions(int* count) {
    if (count) *count = static_cast<int>(sizeof(kTeredoOptions) / sizeof(kTeredoOptions[0]));
    return kTeredoOptions;
}

std::wstring TeredoShowState(std::wstring* output, bool* ok) {
    std::wstring text;
    DWORD code = 0;
    const bool started = RunAndCapture(L"cmd.exe /c netsh int teredo show state", &text, &code);
    if (ok) *ok = started && code == 0;
    if (output) *output = text;
    return text;
}

bool TeredoSetState(const std::wstring& netshArgs, std::wstring* output, DWORD* exitCode) {
    const std::wstring command = L"cmd.exe /c netsh int teredo set state " + netshArgs;
    std::wstring text;
    DWORD code = 1;
    const bool started = RunAndCapture(command, &text, &code);
    if (output) *output = text;
    if (exitCode) *exitCode = code;
    return started && code == 0;
}

// ------------------------------------------------------------- elevation

bool IsElevated() {
    BOOL administrator = FALSE;
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID group = nullptr;
    if (AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &group)) {
        CheckTokenMembership(nullptr, group, &administrator);
        FreeSid(group);
    }
    return administrator != FALSE;
}

bool RelaunchElevated(const std::wstring& arguments, std::wstring* error) {
    wchar_t module[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, module,
                                           static_cast<DWORD>(sizeof(module) / sizeof(module[0])));
    if (length == 0) {
        if (error) *error = FormatWin32Error(static_cast<LONG>(GetLastError()));
        return false;
    }

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = module;
    info.lpParameters = arguments.empty() ? nullptr : arguments.c_str();
    info.nShow = SW_SHOWNORMAL;

    if (ShellExecuteExW(&info)) {
        if (info.hProcess) CloseHandle(info.hProcess);
        if (error) error->clear();
        return true;
    }

    const DWORD code = GetLastError();
    if (error) {
        if (code == ERROR_CANCELLED)
            *error = L"elevation was cancelled";
        else
            *error = FormatWin32Error(static_cast<LONG>(code));
    }
    return false;
}

}  // namespace gtc
