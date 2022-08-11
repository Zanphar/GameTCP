@echo off

@echo Step 1
netsh int teredo set state disabled

@echo Step 2
netsh int teredo set state type=default

@echo Step 3
netsh int teredo set state enterpriseclient

@echo Step 4
netsh int teredo set state servername=teredo.remlab.net

@echo Step 5
netsh interface Teredo set state client clientport=60209

netsh int teredo show stat