#inspired by
#https://blog.daiyanyingyu.uk/2018/03/20/powershell-mtp/
#
# usage: ./MTPdir -Path "Teensy/sdio/dir0/dir1/dir2"
#
#
param([string]$Path)

if ([string]::IsNullOrEmpty($Path)) {
	powershell -noexit -file $MyInvocation.MyCommand.Path -Path "Teensy"
	return
}
echo "Command is ./MTPdir -Path $Path"

$pathParts = @( $Path.Split('/'))

$shell = new-object -com Shell.Application
# 17 (0x11) = ssfDRIVES from the ShellSpecialFolderConstants (https://msdn.microsoft.com/en-us/library/windows/desktop/bb774096(v=vs.85).aspx)
# => "My Computer" — the virtual folder that contains everything on the local computer: storage devices, printers, and Control Panel.
# This folder can also contain mapped network drives.
$shellItem = $shell.NameSpace(17).self

$C1 = $shellItem
foreach ($dir in $pathParts)
{
	$C1 =  @($C1.GetFolder.Items()| where { $_.Name -match $dir })
}
echo @($C1.GetFolder.Items()).name

