$ErrorActionPreference='Stop'
$Host.UI.RawUI.WindowTitle='Lab2 B 2412224 - final evidence'
$e='C:/Users/18504/Desktop/OS/_lab2_B_2412224_20261023/submission/report/B-best-fit/evidence/final-20261024'
New-Item -ItemType Directory -Path $e -Force | Out-Null
function Wait-Capture([string]$phase) {
 $watch=New-Object System.IO.FileSystemWatcher($e,"$phase.continue")
 $watch.EnableRaisingEvents=$true
 Write-Host 'Paused for actual screenshot capture; waiting for external signal.'
 $res=$watch.WaitForChanged([System.IO.WatcherChangeTypes]::Created,600000)
 $watch.Dispose()
 if($res.TimedOut){throw 'Screenshot wait timed out'}
}
Write-Host 'ACTUAL CLEAN AND BUILD - Wang Bokai 2412224'
& wsl.exe -d Ubuntu-22.04 -e bash /mnt/c/Users/18504/Desktop/OS/_lab2_B_2412224_20261023/final-phase.sh clean
& wsl.exe -d Ubuntu-22.04 -e bash /mnt/c/Users/18504/Desktop/OS/_lab2_B_2412224_20261023/final-phase.sh build
if($LASTEXITCODE -ne 0){Wait-Capture 'build';exit 1}
Clear-Host
Write-Host 'ACTUAL QEMU RUN - Wang Bokai 2412224'
Write-Host 'BASE 87f3c095f65ab14920ed60995d9f4c192332c4f1'
& wsl.exe -d Ubuntu-22.04 -e bash /mnt/c/Users/18504/Desktop/OS/_lab2_B_2412224_20261023/final-phase.sh qemu
Wait-Capture 'qemu'
Clear-Host
Write-Host 'ACTUAL FORMAL MAKE GRADE - Wang Bokai 2412224'
Write-Host 'BASE 87f3c095f65ab14920ed60995d9f4c192332c4f1'
& wsl.exe -d Ubuntu-22.04 -e bash /mnt/c/Users/18504/Desktop/OS/_lab2_B_2412224_20261023/final-phase.sh grade
Write-Host 'ACTUAL SERIAL OUTPUT FROM THIS SAME GRADE RUN'
Get-Content -Encoding UTF8 ($e+'/grade-serial.log')
Wait-Capture 'grade'
Write-Host 'Capture completed. No commit or push.'
