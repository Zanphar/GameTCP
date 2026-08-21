Imports Microsoft.Win32
Imports System
Imports System.Diagnostics
Imports System.Security.Principal

' ============================================================================
' GameTCP3 for Microsoft Windows 10/11
' A comprehensive TCP/IP optimization tool for gaming and streaming.
'
' Unified successor to GameTCP and GameTCP2. Combines all optimizations into a
' single tool with 80+ registry tweaks, recursive interface configuration, and
' network stack reset.
'
' Requirements:
'   - Windows 10 or Windows 11
'   - Administrator privileges (required for registry modifications)
'   - .NET Framework (included with Windows)
'
' Usage:
'   - GameSet.bat  - Launches GameTCP3 with admin privileges (UAC elevation)
'   - teredo.bat   - Interactive Teredo tunneling configuration menu
'
' What It Modifies:
'   - Game task scheduling (affinity, clock rate, GPU/CPU priority)
'   - Multimedia system profile (network throttling, system responsiveness)
'   - TCP/IP parameters (Nagle's algorithm, TTL, window size, port range)
'   - Per-interface TCP/IP settings (ACK frequency, window size, MTU)
'   - DNS cache optimization
'   - LanmanServer and LanmanWorkstation parameters
'   - NetBT interface configuration
'   - Internet connection settings (HKCU and HKLM)
'   - Policy and miscellaneous system settings
'   - Memory management
'   - MSMQ settings
'   - Network stack reset via netsh commands
'
' A system reboot is required after applying changes.
'
' Build with: vbc /target:exe /platform:x64 /optimize+ /out:GameTCP3.exe GameTCP3.vb
' Or run Create.bat.
'
' Copyright (c) 2022 Charles McDonald. All rights reserved.
'
' SOFTWARE LICENSE AGREEMENT (BSD 3-Clause License)
'
' Redistribution and use in source and binary forms, with or without
' modification, are permitted provided that the following conditions are met:
'
' 1. Redistributions of source code must retain the above copyright notice,
'    this list of conditions and the following disclaimer.
'
' 2. Redistributions in binary form must reproduce the above copyright notice,
'    this list of conditions and the following disclaimer in the documentation
'    and/or other materials provided with the distribution.
'
' 3. Neither the name of the copyright holder nor the names of its contributors
'    may be used to endorse or promote products derived from this software
'    without specific prior written permission.
'
' THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
' IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
' DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
' FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
' DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
' SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
' CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
' OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
' OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
' ============================================================================

Module GameTCP3

    Private _changedCount As Integer = 0
    Private _unchangedCount As Integer = 0
    Private _failedCount As Integer = 0

    Public Sub Main()

        If Not IsRunningAsAdministrator() Then
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine("Administrator privileges are required. Please run as Administrator.")
            Console.ResetColor()
            Console.ReadKey()
            Return
        End If

        Console.Clear()

        Dim copyright As String = $"Copyright (C) 2022-2026 Charles McDonald. All rights reserved."
        Console.WriteLine(copyright)
        Console.ForegroundColor = ConsoleColor.Green
        Console.WriteLine("GameTCP3 - Network Optimization Tool")
        Console.ResetColor()
        Console.WriteLine("Applying registry settings...")
        Console.WriteLine()

        ApplyTcpipParameters()
        ApplyInterfaceSettings()
        ApplyDnsCacheSettings()
        ApplyServiceProviderSettings()
        ApplyLanmanServerSettings()
        ApplyLanmanWorkstationSettings()
        ApplyMultimediaSettings()
        ApplyGamesSettings()
        ApplyMemoryManagementSettings()
        ApplyMsmqSettings()
        ApplyNetbtSettings()
        ApplyInternetSettings()
        ApplyPoliciesSettings()
        ApplyMiscSettings()

        Console.WriteLine()
        If _failedCount > 0 Then
            Console.ForegroundColor = ConsoleColor.Yellow
            Console.WriteLine($"Registry settings applied with {_changedCount} changed, {_unchangedCount} unchanged, {_failedCount} failed.")
            Console.ResetColor()
        Else
            Console.ForegroundColor = ConsoleColor.Green
            Console.WriteLine("Registry settings applied successfully.")
            Console.ResetColor()
        End If

        ResetNetworkStack()

        Console.WriteLine()
        Console.ForegroundColor = ConsoleColor.Yellow
        Console.BackgroundColor = ConsoleColor.Red
        Console.WriteLine("All operations completed. A reboot is recommended.")
        Console.ResetColor()

    End Sub

    Private Function IsRunningAsAdministrator() As Boolean
        Dim identity As WindowsIdentity = WindowsIdentity.GetCurrent()
        Dim principal As New WindowsPrincipal(identity)
        Return principal.IsInRole(WindowsBuiltInRole.Administrator)
    End Function

    Private Sub ApplyRegistryBatch(path As String, settings As Object()(), Optional sectionName As String = "")
        If sectionName.Length > 0 Then
            Console.ForegroundColor = ConsoleColor.White
            Console.WriteLine($"  [{sectionName}]")
            Console.ResetColor()
        End If

        For Each setting In settings
            Dim valueName As String = CStr(setting(0))
            Dim newValue As Integer = CInt(setting(1))

            Try
                Dim readValue = My.Computer.Registry.GetValue(path, valueName, Nothing)

                Dim oldValueStr As String
                If readValue Is Nothing Then
                    oldValueStr = "(not set)"
                Else
                    oldValueStr = readValue.ToString()
                End If

                My.Computer.Registry.SetValue(path, valueName, newValue, RegistryValueKind.DWord)

                If oldValueStr = newValue.ToString() Then
                    _unchangedCount += 1
                    Console.ForegroundColor = ConsoleColor.DarkGray
                    Console.WriteLine($"    {valueName} = {newValue} (unchanged)")
                Else
                    _changedCount += 1
                    Console.ForegroundColor = ConsoleColor.Cyan
                    Console.Write($"    {valueName}: ")
                    Console.ForegroundColor = ConsoleColor.White
                    Console.Write(oldValueStr)
                    Console.ForegroundColor = ConsoleColor.Cyan
                    Console.Write(" -> ")
                    Console.ForegroundColor = ConsoleColor.Green
                    Console.WriteLine(newValue)
                End If
                Console.ResetColor()
            Catch ex As Exception
                _failedCount += 1
                Console.ForegroundColor = ConsoleColor.Red
                Console.WriteLine($"    Failed to set {valueName}: {ex.Message}")
                Console.ResetColor()
            End Try
        Next
    End Sub

    Private Sub ApplyTcpipParameters()
        Const path As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters"

        Dim settings As Object()() = {
            New Object() {"DefaultTTL", 255},
            New Object() {"TcpTimedWaitDelay", 30},
            New Object() {"TcpFinWait2Delay", 30},
            New Object() {"TcpMaxDupAcks", 2},
            New Object() {"TcpMaxSendFree", 65535},
            New Object() {"StrictTimeWaitSeqCheck", 1},
            New Object() {"MaxFreeTcbs", 65536},
            New Object() {"MaxUserPort", 65534},
            New Object() {"MaxHashTableSize", 65536},
            New Object() {"EnablePMTUBHDetect", 1},
            New Object() {"EnablePMTUDiscovery", 1},
            New Object() {"EnableICMPRedirect", 0},
            New Object() {"SackOpts", 1},
            New Object() {"UseDomainNameDevolution", 1},
            New Object() {"EnableIPAutoConfigurationLimits", 0},
            New Object() {"SynAttackProtect", 1},
            New Object() {"DontAddDefaultGatewayDefault", 1},
            New Object() {"DeadGWDetectDefault", 1},
            New Object() {"EnableConnectionRateLimiting", 0},
            New Object() {"TcpNumConnections", 16777214},
            New Object() {"TcpMaxConnectResponseRetransmissions", 1},
            New Object() {"PPTPTcpMaxDataRetransmissions", 2},
            New Object() {"TcpMaxDataRetransmissions", 7},
            New Object() {"TcpMaxConnectRetransmissions", 8},
            New Object() {"TcpCreateAndConnectTcbRateLimitDepth", 0},
            New Object() {"DisableTaskOffload", 0},
            New Object() {"EnableTCPA", 1},
            New Object() {"EnableWsd", 0},
            New Object() {"EnableDCA", 1},
            New Object() {"ArpRetryCount", 3},
            New Object() {"ArpTRSingleRoute", 1},
            New Object() {"IGMPLevel", 1},
            New Object() {"TcpMaxPortsExhausted", 65535},
            New Object() {"TcpMaxHalfOpen", 200},
            New Object() {"TcpMaxHalfOpenRetried", 80},
            New Object() {"FFPControlFlags", 1},
            New Object() {"TrFunctionalMcastAddress", 0},
            New Object() {"ArpCacheLife", 100},
            New Object() {"ArpCacheMinReferencedLife", 900},
            New Object() {"Tcp1323Opts", 0},
            New Object() {"GlobalMaxTcpWindowSize", 838040},
            New Object() {"NumTcbTablePartitions", 16}
        }

        ApplyRegistryBatch(path, settings, "TCP/IP Parameters")
    End Sub

    Private Sub ApplyInterfaceSettings()
        Const basePath As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces"

        Dim totalCount As Integer = 0

        Console.ForegroundColor = ConsoleColor.White
        Console.WriteLine("  [Network Interface Settings]")
        Console.ResetColor()

        Dim interfaceSettings As Object()() = {
            New Object() {"TCPNoDelay", 1},
            New Object() {"TcpAckFrequency", 1},
            New Object() {"TcpDelAckTicks", 0},
            New Object() {"TcpInitialRTT", 2},
            New Object() {"DontAddDefaultGateway", 0},
            New Object() {"InterfaceMetric", 50},
            New Object() {"TcpWindowSize", 838040},
            New Object() {"MTU", 1500}
        }

        Try
            Using baseKey As RegistryKey = Registry.LocalMachine.OpenSubKey(
                "SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\Interfaces")
                If baseKey IsNot Nothing Then
                    ApplyInterfaceSettingsRecursive(baseKey, basePath, interfaceSettings, totalCount)
                End If
            End Using
        Catch ex As Exception
            _failedCount += 1
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine($"  Failed to enumerate network interfaces: {ex.Message}")
            Console.ResetColor()
            Return
        End Try

        Console.ForegroundColor = ConsoleColor.Green
        Console.WriteLine($"    Applied settings to {totalCount} interface(s)")
        Console.ResetColor()
    End Sub

    Private Sub ApplyInterfaceSettingsRecursive(key As RegistryKey, basePath As String, interfaceSettings As Object()(), ByRef totalCount As Integer)
        For Each subKeyName In key.GetSubKeyNames()
            Dim subKeyPath As String = $"{basePath}\{subKeyName}"

            Try
                Using subKey As RegistryKey = key.OpenSubKey(subKeyName, True)
                    If subKey Is Nothing Then Continue For

                    Dim hasChildren As Boolean = subKey.SubKeyCount > 0
                    Dim hasValues As Boolean = subKey.ValueCount > 0

                    If hasValues Then
                        totalCount += 1
                        For Each setting In interfaceSettings
                            Dim valueName As String = CStr(setting(0))
                            Dim newValue As Integer = CInt(setting(1))

                            Try
                                Dim readValue = My.Computer.Registry.GetValue(subKeyPath, valueName, Nothing)
                                My.Computer.Registry.SetValue(subKeyPath, valueName, newValue, RegistryValueKind.DWord)

                                Dim oldValueStr As String = If(readValue Is Nothing, "(not set)", readValue.ToString())
                                If oldValueStr <> newValue.ToString() Then
                                    _changedCount += 1
                                    Console.ForegroundColor = ConsoleColor.DarkGray
                                    Console.WriteLine($"    [{subKeyName}] {valueName}: {oldValueStr} -> {newValue}")
                                    Console.ResetColor()
                                Else
                                    _unchangedCount += 1
                                End If
                            Catch ex As Exception
                                _failedCount += 1
                                Console.ForegroundColor = ConsoleColor.Red
                                Console.WriteLine($"    Failed to set {valueName} on {subKeyName}: {ex.Message}")
                                Console.ResetColor()
                            End Try
                        Next
                    End If

                    If hasChildren Then
                        ApplyInterfaceSettingsRecursive(subKey, subKeyPath, interfaceSettings, totalCount)
                    End If
                End Using
            Catch ex As Exception
                _failedCount += 1
                Console.ForegroundColor = ConsoleColor.Red
                Console.WriteLine($"    Failed to access {subKeyPath}: {ex.Message}")
                Console.ResetColor()
            End Try
        Next
    End Sub

    Private Sub ApplyDnsCacheSettings()
        Const path As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Dnscache\Parameters"

        Dim settings As Object()() = {
            New Object() {"MaxCacheEntryTtlLimit", 64000},
            New Object() {"MaxSOACacheEntryTtlLimit", 301},
            New Object() {"NegativeCacheTime", 0},
            New Object() {"NetFailureCacheTime", 0},
            New Object() {"NegativeSOACacheTime", 0},
            New Object() {"CacheHashTableBucketSize", 1},
            New Object() {"CacheHashTableSize", 384}
        }

        ApplyRegistryBatch(path, settings, "DNS Cache Settings")
    End Sub

    Private Sub ApplyServiceProviderSettings()
        Const path As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\ServiceProvider"

        Dim settings As Object()() = {
            New Object() {"DnsPriority", 6},
            New Object() {"HostsPriority", 5},
            New Object() {"LocalPriority", 4},
            New Object() {"NetbtPriority", 7}
        }

        ApplyRegistryBatch(path, settings, "Service Provider Settings")
    End Sub

    Private Sub ApplyLanmanServerSettings()
        Const path As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\LanmanServer\Parameters"

        Dim settings As Object()() = {
            New Object() {"Size", 3},
            New Object() {"IRPStackSize", 50},
            New Object() {"SizReqBuf", 17424}
        }

        ApplyRegistryBatch(path, settings, "LanmanServer Settings")
    End Sub

    Private Sub ApplyLanmanWorkstationSettings()
        Const path As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\LanmanWorkstation\Parameters"

        Dim settings As Object()() = {
            New Object() {"KeepConn", 32000},
            New Object() {"CollectionTime", 1},
            New Object() {"MaxCollectionCount", 1},
            New Object() {"SizCharBuf", 4096},
            New Object() {"MaxCmds", 255},
            New Object() {"BufFilesDenyWrite", 0},
            New Object() {"DisableLargeMtu", 0},
            New Object() {"DisableBandwidthThrottling", 1}
        }

        ApplyRegistryBatch(path, settings, "LanmanWorkstation Settings")
    End Sub

    Private Sub ApplyMultimediaSettings()
        Const path As String = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile"

        Dim settings As Object()() = {
            New Object() {"NetworkThrottlingIndex", 18309},
            New Object() {"SystemResponsiveness", 2}
        }

        ApplyRegistryBatch(path, settings, "Multimedia Settings")
    End Sub

    Private Sub ApplyGamesSettings()
        Const path As String = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks\Games"

        Dim dwordSettings As Object()() = {
            New Object() {"Affinity", 0},
            New Object() {"Clock Rate", 2710},
            New Object() {"GPU Priority", 8},
            New Object() {"Priority", 6}
        }

        ApplyRegistryBatch(path, dwordSettings, "Games Settings (DWORD)")

        Dim stringSettings As String()() = {
            New String() {"Scheduling Category", "High"},
            New String() {"SFIO Priority", "High"},
            New String() {"Background Only", "False"}
        }

        For Each setting In stringSettings
            Dim valueName As String = setting(0)
            Dim newValue As String = setting(1)

            Try
                Dim readValue = My.Computer.Registry.GetValue(path, valueName, Nothing)
                Dim oldValueStr As String = If(readValue Is Nothing, "(not set)", readValue.ToString())

                My.Computer.Registry.SetValue(path, valueName, newValue)

                If oldValueStr = newValue Then
                    _unchangedCount += 1
                    Console.ForegroundColor = ConsoleColor.DarkGray
                    Console.WriteLine($"    {valueName} = {newValue} (unchanged)")
                Else
                    _changedCount += 1
                    Console.ForegroundColor = ConsoleColor.Cyan
                    Console.Write($"    {valueName}: ")
                    Console.ForegroundColor = ConsoleColor.White
                    Console.Write(oldValueStr)
                    Console.ForegroundColor = ConsoleColor.Cyan
                    Console.Write(" -> ")
                    Console.ForegroundColor = ConsoleColor.Green
                    Console.WriteLine(newValue)
                End If
                Console.ResetColor()
            Catch ex As Exception
                _failedCount += 1
                Console.ForegroundColor = ConsoleColor.Red
                Console.WriteLine($"    Failed to set {valueName}: {ex.Message}")
                Console.ResetColor()
            End Try
        Next
    End Sub

    Private Sub ApplyMemoryManagementSettings()
        Const path As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management"

        Dim settings As Object()() = {
            New Object() {"LargeSystemCache", 0},
            New Object() {"DisablePagingExecutive", 1},
            New Object() {"SecondLevelDataCache", 1024},
            New Object() {"ThirdLevelDataCache", 6144},
            New Object() {"IoPageLockLimit", 983040}
        }

        ApplyRegistryBatch(path, settings, "Memory Management Settings")
    End Sub

    Private Sub ApplyMsmqSettings()
        Const basePath As String = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MSMQ\Parameters"
        Const securityPath As String = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MSMQ\Parameters\Security"

        Console.ForegroundColor = ConsoleColor.White
        Console.WriteLine("  [MSMQ Settings]")
        Console.ResetColor()

        Try
            Dim readValue = My.Computer.Registry.GetValue(basePath, "TCPNoDelay", Nothing)
            My.Computer.Registry.SetValue(basePath, "TCPNoDelay", 1, RegistryValueKind.DWord)
            Dim oldStr = If(readValue Is Nothing, "(not set)", readValue.ToString())
            If oldStr = "1" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    TCPNoDelay = 1 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    TCPNoDelay: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(oldStr)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(1)
            End If
            Console.ResetColor()
        Catch ex As Exception
            _failedCount += 1
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine($"    Failed to set TCPNoDelay: {ex.Message}")
            Console.ResetColor()
        End Try

        Try
            Dim readValue = My.Computer.Registry.GetValue(securityPath, "SecureDSCommunication", Nothing)
            My.Computer.Registry.SetValue(securityPath, "SecureDSCommunication", 0, RegistryValueKind.DWord)
            Dim oldStr = If(readValue Is Nothing, "(not set)", readValue.ToString())
            If oldStr = "0" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    SecureDSCommunication = 0 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    SecureDSCommunication: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(oldStr)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(0)
            End If
            Console.ResetColor()
        Catch ex As Exception
            _failedCount += 1
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine($"    Failed to set SecureDSCommunication: {ex.Message}")
            Console.ResetColor()
        End Try

        Try
            Dim readValue = My.Computer.Registry.GetValue(securityPath, "LargeSystemCache", Nothing)
            My.Computer.Registry.SetValue(securityPath, "LargeSystemCache", 0, RegistryValueKind.DWord)
            Dim oldStr = If(readValue Is Nothing, "(not set)", readValue.ToString())
            If oldStr = "0" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    LargeSystemCache = 0 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    LargeSystemCache: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(oldStr)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(0)
            End If
            Console.ResetColor()
        Catch ex As Exception
            _failedCount += 1
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine($"    Failed to set LargeSystemCache: {ex.Message}")
            Console.ResetColor()
        End Try
    End Sub

    Private Sub ApplyNetbtSettings()
        Const path As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\NetBT\Parameters"

        Dim settings As Object()() = {
            New Object() {"EnableDns", 0},
            New Object() {"BcastNameQueryCount", 3},
            New Object() {"EnableLMHOSTS", 1},
            New Object() {"NameSrvQueryCount", 3},
            New Object() {"UseNewSmb", 1},
            New Object() {"EnableProxy", 0},
            New Object() {"EnableProxyRegCheck", 0}
        }

        ApplyRegistryBatch(path, settings, "NetBT Settings")
        ApplyNetbtInterfaceSettings()
    End Sub

    Private Sub ApplyNetbtInterfaceSettings()
        Const basePath As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\NetBT\Parameters\Interfaces"

        Console.ForegroundColor = ConsoleColor.White
        Console.WriteLine("  [NetBT Interface Settings]")
        Console.ResetColor()

        Dim interfaceCount As Integer = 0

        Try
            Dim interfaces As String() = Nothing
            Using baseKey As RegistryKey = Registry.LocalMachine.OpenSubKey(
                "SYSTEM\CurrentControlSet\Services\NetBT\Parameters\Interfaces")
                If baseKey IsNot Nothing Then
                    interfaces = baseKey.GetSubKeyNames()
                End If
            End Using

            If interfaces Is Nothing OrElse interfaces.Length = 0 Then
                Console.ForegroundColor = ConsoleColor.Yellow
                Console.WriteLine("    No NetBT interfaces found.")
                Console.ResetColor()
                Return
            End If

            For Each iface In interfaces
                Try
                    Dim ifacePath = $"{basePath}\{iface}"
                    Dim readValue = My.Computer.Registry.GetValue(ifacePath, "NetbiosOptions", Nothing)
                    My.Computer.Registry.SetValue(ifacePath, "NetbiosOptions", 1, RegistryValueKind.DWord)
                    Dim oldStr = If(readValue Is Nothing, "(not set)", readValue.ToString())
                    If oldStr <> "1" Then
                        _changedCount += 1
                        Console.ForegroundColor = ConsoleColor.DarkGray
                        Console.WriteLine($"    [{iface}] NetbiosOptions: {oldStr} -> 1")
                        Console.ResetColor()
                    Else
                        _unchangedCount += 1
                    End If
                    interfaceCount += 1
                Catch ex As Exception
                    _failedCount += 1
                    Console.ForegroundColor = ConsoleColor.Red
                    Console.WriteLine($"    Failed to set NetbiosOptions on {iface}: {ex.Message}")
                    Console.ResetColor()
                End Try
            Next
        Catch ex As Exception
            _failedCount += 1
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine($"    Failed to enumerate NetBT interfaces: {ex.Message}")
            Console.ResetColor()
        End Try

        Console.ForegroundColor = ConsoleColor.Green
        Console.WriteLine($"    Applied settings to {interfaceCount} NetBT interface(s)")
        Console.ResetColor()
    End Sub

    Private Sub ApplyInternetSettings()
        Const hkcuPath As String = "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings"
        Const hklmPath As String = "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Internet Settings"

        Console.ForegroundColor = ConsoleColor.White
        Console.WriteLine("  [Internet Settings]")
        Console.ResetColor()

        Try
            Dim readVal1 = My.Computer.Registry.GetValue(hkcuPath, "MaxConnectionsPerServer", Nothing)
            Dim readVal2 = My.Computer.Registry.GetValue(hkcuPath, "MaxConnectionsPer1_0Server", Nothing)
            My.Computer.Registry.SetValue(hkcuPath, "MaxConnectionsPerServer", 69, RegistryValueKind.DWord)
            My.Computer.Registry.SetValue(hkcuPath, "MaxConnectionsPer1_0Server", 69, RegistryValueKind.DWord)
            Dim old1 = If(readVal1 Is Nothing, "(not set)", readVal1.ToString())
            Dim old2 = If(readVal2 Is Nothing, "(not set)", readVal2.ToString())

            If old1 = "69" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    HKCU MaxConnectionsPerServer = 69 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    HKCU MaxConnectionsPerServer: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(old1)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(69)
            End If

            If old2 = "69" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    HKCU MaxConnectionsPer1_0Server = 69 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    HKCU MaxConnectionsPer1_0Server: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(old2)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(69)
            End If
            Console.ResetColor()
        Catch ex As Exception
            _failedCount += 1
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine($"    Failed to set HKCU Internet settings: {ex.Message}")
            Console.ResetColor()
        End Try

        Try
            Dim readVal1 = My.Computer.Registry.GetValue(hklmPath, "MaxConnectionsPerServer", Nothing)
            Dim readVal2 = My.Computer.Registry.GetValue(hklmPath, "MaxConnectionsPer1_0Server", Nothing)
            Dim readVal3 = My.Computer.Registry.GetValue(hklmPath, "SmoothScroll", Nothing)
            My.Computer.Registry.SetValue(hklmPath, "MaxConnectionsPerServer", 16, RegistryValueKind.DWord)
            My.Computer.Registry.SetValue(hklmPath, "MaxConnectionsPer1_0Server", 16, RegistryValueKind.DWord)
            My.Computer.Registry.SetValue(hklmPath, "SmoothScroll", 1, RegistryValueKind.DWord)
            Dim old1 = If(readVal1 Is Nothing, "(not set)", readVal1.ToString())
            Dim old2 = If(readVal2 Is Nothing, "(not set)", readVal2.ToString())
            Dim old3 = If(readVal3 Is Nothing, "(not set)", readVal3.ToString())

            If old1 = "16" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    HKLM MaxConnectionsPerServer = 16 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    HKLM MaxConnectionsPerServer: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(old1)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(16)
            End If

            If old2 = "16" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    HKLM MaxConnectionsPer1_0Server = 16 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    HKLM MaxConnectionsPer1_0Server: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(old2)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(16)
            End If

            If old3 = "1" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    HKLM SmoothScroll = 1 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    HKLM SmoothScroll: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(old3)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(1)
            End If
            Console.ResetColor()
        Catch ex As Exception
            _failedCount += 1
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine($"    Failed to set HKLM Internet settings: {ex.Message}")
            Console.ResetColor()
        End Try
    End Sub

    Private Sub ApplyPoliciesSettings()
        Console.ForegroundColor = ConsoleColor.White
        Console.WriteLine("  [Policy Settings]")
        Console.ResetColor()

        Dim policies As Object()() = {
            New Object() {"HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows\Psched", "NonBestEffortLimit", 0},
            New Object() {"HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows", "NonBestEffortLimit", 0}
        }

        For Each policy In policies
            Dim regPath As String = CStr(policy(0))
            Dim valueName As String = CStr(policy(1))
            Dim newValue As Integer = CInt(policy(2))

            Try
                Dim readValue = My.Computer.Registry.GetValue(regPath, valueName, Nothing)
                My.Computer.Registry.SetValue(regPath, valueName, newValue, RegistryValueKind.DWord)
                Dim oldStr = If(readValue Is Nothing, "(not set)", readValue.ToString())

                If oldStr = newValue.ToString() Then
                    _unchangedCount += 1
                    Console.ForegroundColor = ConsoleColor.DarkGray
                    Console.WriteLine($"    {valueName} = {newValue} (unchanged)")
                Else
                    _changedCount += 1
                    Console.ForegroundColor = ConsoleColor.Cyan
                    Console.Write($"    {valueName}: ")
                    Console.ForegroundColor = ConsoleColor.White
                    Console.Write(oldStr)
                    Console.ForegroundColor = ConsoleColor.Cyan
                    Console.Write(" -> ")
                    Console.ForegroundColor = ConsoleColor.Green
                    Console.WriteLine(newValue)
                End If
                Console.ResetColor()
            Catch ex As Exception
                _failedCount += 1
                Console.ForegroundColor = ConsoleColor.Red
                Console.WriteLine($"    Failed to set {valueName}: {ex.Message}")
                Console.ResetColor()
            End Try
        Next
    End Sub

    Private Sub ApplyMiscSettings()
        Console.ForegroundColor = ConsoleColor.White
        Console.WriteLine("  [Miscellaneous Settings]")
        Console.ResetColor()

        Try
            Const path As String = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Reliability"
            Dim readValue = My.Computer.Registry.GetValue(path, "TimeStampInterval", Nothing)
            My.Computer.Registry.SetValue(path, "TimeStampInterval", 0, RegistryValueKind.DWord)
            Dim oldStr = If(readValue Is Nothing, "(not set)", readValue.ToString())
            If oldStr = "0" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    TimeStampInterval = 0 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    TimeStampInterval: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(oldStr)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(0)
            End If
            Console.ResetColor()
        Catch ex As Exception
            _failedCount += 1
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine($"    Failed to set TimeStampInterval: {ex.Message}")
            Console.ResetColor()
        End Try

        Try
            Const path As String = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\BITS"
            Dim readValue = My.Computer.Registry.GetValue(path, "Start", Nothing)
            My.Computer.Registry.SetValue(path, "Start", 3, RegistryValueKind.DWord)
            Dim oldStr = If(readValue Is Nothing, "(not set)", readValue.ToString())
            If oldStr = "3" Then
                _unchangedCount += 1
                Console.ForegroundColor = ConsoleColor.DarkGray
                Console.WriteLine("    BITS Start = 3 (unchanged)")
            Else
                _changedCount += 1
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write("    BITS Start: ")
                Console.ForegroundColor = ConsoleColor.White
                Console.Write(oldStr)
                Console.ForegroundColor = ConsoleColor.Cyan
                Console.Write(" -> ")
                Console.ForegroundColor = ConsoleColor.Green
                Console.WriteLine(3)
            End If
            Console.ResetColor()
        Catch ex As Exception
            _failedCount += 1
            Console.ForegroundColor = ConsoleColor.Red
            Console.WriteLine($"    Failed to set BITS Start: {ex.Message}")
            Console.ResetColor()
        End Try
    End Sub

    Private Sub ResetNetworkStack()

        Console.WriteLine()
        Console.ForegroundColor = ConsoleColor.White
        Console.WriteLine("Resetting network stack...")
        Console.ResetColor()

        Dim commands As String() = {
            "netsh int tcp reset",
            "netsh int ipv4 reset",
            "netsh int ipv6 reset",
            "netsh int ip reset",
            "netsh winsock reset catalog",
            "netsh int ip delete neighbors",
            "netsh Int tcp set global nonsackrttresiliency=disabled",
            "netsh Int tcp set global chimney=disabled",
            "netsh Int tcp set global netdma=enabled",
            "netsh interface ip delete arpcache",
            "netsh Int tcp set global rss=enabled",
            "netsh Int tcp set supplemental congestionprovider=ctcp",
            "netsh Int tcp set global initialRto=2000",
            "netsh Int tcp set global dca=enabled",
            "netsh interface tcp set global ecncapability=disabled",
            "netsh int tcp set global autotuninglevel=normal",
            "netsh int tcp set global rsc=disabled",
            "netsh Int ipv4 set glob defaultcurhoplimit=255",
            "netsh Int tcp set global maxsynretransmissions=2",
            "netsh Int ipv6 set glob defaultcurhoplimit=255",
            "netsh int tcp set heuristics disabled",
            "netsh int tcp set global timestamps=disabled",
            "netsh int tcp set global fastopen=disabled"
        }

        Dim cmdFailed As Integer = 0
        Dim cmdSucceeded As Integer = 0

        For Each cmd In commands
            Try
                Using proc As New Process()
                    proc.StartInfo = New ProcessStartInfo() With {
                        .FileName = "cmd.exe",
                        .Arguments = $"/c {cmd}",
                        .UseShellExecute = False,
                        .CreateNoWindow = True,
                        .RedirectStandardOutput = True,
                        .RedirectStandardError = True
                    }
                    proc.Start()
                    Dim stdOut As String = proc.StandardOutput.ReadToEnd()
                    Dim stdErr As String = proc.StandardError.ReadToEnd()
                    proc.WaitForExit()

                    If proc.ExitCode <> 0 Then
                        cmdFailed += 1
                        Console.ForegroundColor = ConsoleColor.Red
                        Console.WriteLine($"    Failed: {cmd}")
                        If stdErr.Trim().Length > 0 Then
                            Console.WriteLine($"      Error: {stdErr.Trim()}")
                        End If
                        Console.ResetColor()
                    Else
                        cmdSucceeded += 1
                    End If
                End Using
            Catch ex As Exception
                cmdFailed += 1
                Console.ForegroundColor = ConsoleColor.Red
                Console.WriteLine($"    Failed to run: {cmd} - {ex.Message}")
                Console.ResetColor()
            End Try
        Next

        Console.WriteLine()
        If cmdFailed > 0 Then
            Console.ForegroundColor = ConsoleColor.Yellow
            Console.WriteLine($"Network stack reset completed with {cmdSucceeded} succeeded, {cmdFailed} failed.")
            Console.ResetColor()
        Else
            Console.ForegroundColor = ConsoleColor.Green
            Console.WriteLine("Network stack reset complete.")
            Console.ResetColor()
        End If
    End Sub

End Module
