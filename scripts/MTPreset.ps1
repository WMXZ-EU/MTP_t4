if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process PowerShell -Verb RunAs "-NoProfile -ExecutionPolicy Bypass -Command `"cd '$pwd'; & '$PSCommandPath';`"";
    exit;
}

$a = Get-WmiObject Win32_PNPEntity | where { $_.Name -eq 'Teensy'}

echo $a.Name
$a | Disable-PnpDevice -Confirm:$false;
$a | Enable-PnpDevice -Confirm:$false;
