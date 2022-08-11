@echo off
vbc GameTCP.vb
vbc GameTCP2.vb
mt.exe -nologo -manifest ".\GameTCP.exe.manifest" -outputresource:".\GameTCP.exe;#1"
mt.exe -nologo -manifest ".\GameTCP2.exe.manifest" -outputresource:".\GameTCP2.exe;#1"
