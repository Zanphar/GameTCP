Imports System
Imports Microsoft.VisualBasic.FileIO
Imports Microsoft.Win32
Imports System.Diagnostics
Imports System.IO

' GameTCP (HS) for Microsoft Windows 10 - A simple program to make changes to Microsoft Windows 10 systems to increase performance, 
' and to improve performance for gaming and other applications. This version is independant and special from GameTCP, therefore it
' is not likely they'll be following each other, except in some small areas.
' 
' Visual Basic (Visual Studio .NET 2019 Community Edition) is used to compile this source code. Visual Studio .NET 2019 
' Community Edition is available free of charge from Microsoft.
' 
' 1.) Intel(R) Xeon(R) CPU E5-2670 0 @ 2.60GHz (8 Cores; 16 Threads)
' 2.) AMD RADEON RX 470 4GB GDDR5 RAM
' 3.) 16.0 GB DDR3
' 4.) 64-bit OS, x64-based CPU
' 5.) Windows 10 Professional
' 6.) T-Mobile Home Internet (5G/LTE)
' 
' SOFTWARE LICENSE AGREEMENT
' Copyright © 2022 Charles McDonald.
' 
' Redistribution and use in source and binary forms, with or without modification, are permitted provided
' that the following conditions are met:
' 
' 1. Redistributions of source code must retain the above copyright notice, this list of conditions and
' the following disclaimer.
' 
' 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
' and the following disclaimer in the documentation and/or other materials provided with the distribution.
' 
' 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse
' or promote products derived from this software without specific prior written permission.
' 
' THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
' WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
' PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
' FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
' LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
' INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
' OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
' IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
' 
' netsh int tcp set global rsc=disabled

Module GameTCP

Public Sub Main()

	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "DefaultTTL", 255, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpTimedWaitDelay", 30, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpFinWait2Delay", 30, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpMaxDupAcks", 2, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpMaxSendFree", 65535, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "StrictTimeWaitSeqCheck", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "MaxFreeTcbs", 65536, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "MaxUserPort", 65534, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "MaxHashTableSize", 65536, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "EnablePMTUBHDetect", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "EnablePMTUDiscovery", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "EnableICMPRedirect", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "SackOpts", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "UseDomainNameDevolution", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "EnableIPAutoConfigurationLimits", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "SynAttackProtect", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "DontAddDefaultGatewayDefault", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "DeadGWDetectDefault", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "EnableConnectionRateLimiting", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpNumConnections", 16777214, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpMaxConnectResponseRetransmissions", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "PPTPTcpMaxDataRetransmissions", 2, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpMaxDataRetransmissions", 7, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpMaxConnectRetransmissions", 8, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpCreateAndConnectTcbRateLimitDepth", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "DisableTaskOffload", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "EnableTCPA", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "EnableWsd", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "EnableDCA", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "ArpRetryCount", 3, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "ArpTRSingleRoute", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "IGMPLevel", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpMaxPortsExhausted", 65535, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpMaxHalfOpen", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TcpMaxHalfOpenRetried", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "FFPControlFlags", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "TrFunctionalMcastAddress", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "MaxCacheEntryTtlLimit", 64000, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "MaxSOACacheEntryTtlLimit", 301, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "NegativeCacheTime", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "NetFailureCacheTime", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "NegativeSOACacheTime", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider", "DnsPriority", 6, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider", "HostsPriority", 5, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider", "LocalPriority", 4, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "ArpCacheLife", 100, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "ArpCacheMinReferencedLife", 900, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{d3abf5ef-3c79-4b14-a311-2e4b57a7a086}", "TCPNoDelay", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{d3abf5ef-3c79-4b14-a311-2e4b57a7a086}", "TcpAckFrequency", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{d3abf5ef-3c79-4b14-a311-2e4b57a7a086}", "TcpDelAckTicks", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{d3abf5ef-3c79-4b14-a311-2e4b57a7a086}", "TcpInitialRTT", 2, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{d3abf5ef-3c79-4b14-a311-2e4b57a7a086}", "DontAddDefaultGateway", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "CacheHashTableBucketSize", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", "CacheHashTableSize", 384, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider", "NetbtPriority", 7, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\MSMQ\\Parameters", "TCPNoDelay", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\MSMQ\\Parameters\\Security", "SecureDSCommunication", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\MSMQ\\Parameters\\Security", "LargeSystemCache", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched", "NonBestEffortLimit", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters", "Size", 3, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters", "IRPStackSize", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters", "SizReqBuf", 17424, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "KeepConn", 32000, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "CollectionTime", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "MaxCollectionCount", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "SizCharBuf", 4096, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "MaxCmds", 255, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "BufFilesDenyWrite", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "DisableLargeMtu", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "DisableBandwidthThrottling", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile", "NetworkThrottlingIndex", 18309, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile", "SystemResponsiveness", 2, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "MaxConnectionsPerServer", 69, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "MaxConnectionsPer1_0Server", 69, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", "LargeSystemCache", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", "Size/Small/Medium/Large", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", "EnableDns", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", "BcastNameQueryCount", 3, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", "EnableLMHOSTS", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", "NameSrvQueryCount", 3, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", "UseNewSmb", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", "EnableProxy", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", "EnableProxyRegCheck", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces\\Tcpip_{616fd3ed-a1ed-4e29-b3cb-e1ae0317d1e9}", "NetbiosOptions", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\BITS", "Start", 3, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters", "IRPStackSize", 50, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters", "SizReqBuft", 17424, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters", "Size", 3, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Psched", "NonBestEffortLimit", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\Windows", "NonBestEffortLimit", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability", "TimeStampInterval", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", "DisablePagingExecutive", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", "LargeSystemCache", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "MaxConnectionsPerServer", 16, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "EndMaxConnectionsPer1_0Server", 16, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", "SmoothScroll", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion", "MaxConnectionsPerServer", 16, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion", "EndMaxConnectionsPer1_0Server", 16, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion", "SmoothScroll", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "DisableLargeMtu", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters", "ClassDisableBandwidthThrottling", 1, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{d3abf5ef-3c79-4b14-a311-2e4b57a7a086}", "InterfaceMetric", 50, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "Tcp1323Opts", 0, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "GlobalMaxTcpWindowSize", 838040, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "NumTcbTablePartitions", 16, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{d3abf5ef-3c79-4b14-a311-2e4b57a7a086}", "TcpWindowSize", 838040, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{d3abf5ef-3c79-4b14-a311-2e4b57a7a086}", "MTU", 1500, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", "SecondLevelDataCache", 1024, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", "ThirdLevelDataCache", 6144, RegistryValueKind.DWord)
	My.Computer.Registry.SetValue("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", "IoPageLockLimit", 983040, RegistryValueKind.DWord)
		
	Process.Start("cmd /c netsh int tcp reset & netsh int ipv4 reset & netsh int ipv6 reset & netsh int ip reset & netsh winsock reset catalog & netsh int ip delete neighbors & netsh Int tcp set global nonsackrttresiliency=disabled & netsh Int tcp set global chimney=disabled & netsh Int tcp set global netdma=enabled & netsh interface ip delete arpcache & netsh Int tcp set global rss=enabled & netsh Int tcp set supplemental congestionprovider=ctcp & netsh Int tcp set global initialRto=2000 & netsh Int tcp set global dca=enabled & netsh interface tcp set global ecncapability=disabled & netsh int tcp set global autotuninglevel=normal & netsh Int tcp set global ecncapability=disabled & netsh int tcp set global rsc=disabled & netsh Int ipv4 set glob defaultcurhoplimit=255 & netsh Int tcp set global maxsynretransmissions=2 & netsh Int ipv6 set glob defaultcurhoplimit=255 & netsh int tcp set heuristics disabled & netsh int tcp set global netdma=enabled & netsh int tcp set global dca=enabled & set supplemental congestionprovider=ctcp & netsh int tcp set global timestamps=disabled & netsh int tcp set global nonsackrttresiliency=disabled & netsh int tcp set global fastopen=disabled & netsh int ipv4 set glob defaultcurhoplimit=255 & netsh int ipv6 set glob defaultcurhoplimit=255")
	Process.Start("cmd /c ipconfig /release & ipconfig /flushdns & ipconfig /renew")

	End Sub

End Module
