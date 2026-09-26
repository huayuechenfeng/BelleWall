[CmdletBinding()]
param([string]$Port, [ValidatePattern('^[0-9a-f]{16}$')][string]$SessionToken, [ValidatePattern('^[0-9a-f]{64}\.(bwv|html)(\.json)?$')][string]$LibraryFile, [ValidatePattern('^p[0-9]+$')][string]$ContextId, [ValidateSet('fault-owner','fault-web','pull-library','package-info','pull-diagnostics','probe','status','project-processes','status-details','terminate','start','stop','log','probe-log','native-log','cache-log','render-log','bench-log','pull-screen','pull-render-odt','pull-longrun-odt','native-stop','install','stage-install','pull-rom','pull-snapshot','candidate-log','inventory','pull-project','content-info','stage-sywp','import-sywp')][string]$Action='probe', [string]$Mode='blocks', [ValidateSet('video','web','corrupt','unsupported','pc-made')][string]$Fixture='video', [string]$OutputFile='research/evidence/device/host.log', [ValidateSet('alfdecoderserverclient.dll','alfappservercore.dll','alfrenderstage.dll','alfcompositorrs.dll','aknskins.dll','aknskinsrv.dll','xn3layoutengine.dll','extrenderingplugin.dll','hspsclient.dll','hspsdefrep.dll','hspsthemeserver.exe','hspsclientsession.dll','hsccapiclient.dll')][string]$Module='alfdecoderserverclient.dll', [ValidateSet('bellewall','belleweb','belleprobe','bellepaper','bellecache','bellecontent','bellerender','bellerenderhost')][string]$Package='bellewall', [string]$Checkpoint='dist/daily-candidate-20260920-r2', [ValidateSet('preparation-pending.bin','candidate-session.bin','wallpaper-rollback.bin','pages-rollback.bin','selected-wallpaper.txt','bellepaper.exe','bellerenderhost.exe','bellewall.exe','belleweb.exe','native-wallpaper.previous.log','host.previous.log','candidate-last-result.txt','render-plugin.previous.log','render-longrun.log','render-longrun.previous.log')][string]$ProjectFile='candidate-session.bin')
$ErrorActionPreference='Stop'
# Minimal CODA/TCF USB transport. Protocol reference: Qt Creator v2.4.1
# src/shared/symbianutils/codadevice.cpp. No debug attachment is requested.
Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Collections.Generic;
public sealed class BelleCoda : IDisposable {
 SerialPort port; int token;
 public BelleCoda(string name) { port=new SerialPort(name,115200,Parity.None,8,StopBits.One); port.ReadTimeout=200; port.WriteTimeout=3000; port.Open(); }
 public void Dispose(){port.Dispose();}
 void Send(byte[] b){if(b.Length>1022)throw new Exception("Command too large");byte[] h={1,0x92,(byte)(b.Length>>8),(byte)b.Length};port.Write(h,0,4);port.Write(b,0,b.Length);}
 byte Read(DateTime end){while(DateTime.UtcNow<end){try{return (byte)port.ReadByte();}catch(TimeoutException){}}throw new TimeoutException("CODA response timeout");}
 byte[] Receive(DateTime end){var joined=new List<byte>();while(true){byte a=Read(end);if(a!=1)continue;byte protocol=Read(end);int n=(Read(end)<<8)|Read(end);if(n>32768)throw new Exception("Invalid frame length");byte[] b=new byte[n];for(int i=0;i<n;i++)b[i]=Read(end);if(protocol!=0x92)continue;if(n>2&&(b[0]==0xfe||b[0]==0)){if(b[0]==0xfe)joined.Clear();for(int i=2;i<n;i++)joined.Add(b[i]);if(b[1]==0)return joined.ToArray();continue;}return b;}}
 public string Hello(){Send(new byte[]{0xfc,0x1f});var end=DateTime.UtcNow.AddSeconds(10);byte[] b;do{b=Receive(end);}while(b.Length<2||b[0]!=0xfc||b[1]!=0xf1);string version=Encoding.UTF8.GetString(b,2,b.Length-2);Send(Encoding.UTF8.GetBytes("E\0Locator\0Hello\0[\"Locator\"]\0"));Console.WriteLine("CODA "+version);return Encoding.UTF8.GetString(Receive(end)).Replace('\0','|');}
 public string[] Call(string service,string command,string[] args){string id=(++token).ToString();string wire="C\0"+id+"\0"+service+"\0"+command+"\0";foreach(string arg in args)wire+=arg+"\0";Send(Encoding.UTF8.GetBytes(wire));var end=DateTime.UtcNow.AddSeconds(service=="SymbianInstall"?120:15);while(true){string msg=Encoding.UTF8.GetString(Receive(end));string[] fields=msg.Split('\0');if(fields.Length>1&&fields[1]==id&&(fields[0]=="R"||fields[0]=="N")){if(fields[0]=="N")throw new Exception("Unsupported CODA command: "+command);return fields;}Console.WriteLine("EVENT "+msg.Replace('\0','|'));}}
}
'@
function Invoke-Coda([string]$Service,[string]$Command,[object[]]$Arguments=@()) {
    $encoded=[System.Collections.Generic.List[string]]::new()
    foreach($arg in $Arguments){$encoded.Add((ConvertTo-Json -InputObject $arg -Compress -Depth 20))}
    $reply=$client.Call($Service,$Command,$encoded.ToArray())
    if($Command -notin @('read','write')){Write-Host ($Service+'.'+$Command+' -> '+($reply -join '|'))}
    if($Command -ne 'read' -and $reply[2] -and $reply[2] -ne 'null'){throw ('CODA error: '+$reply[2])}
    return ,$reply
}
if(-not $Port){throw 'Specify the currently verified port explicitly'}
$client=[BelleCoda]::new($Port)
try {
    Write-Host $client.Hello()
    if($Action -eq 'package-info') {
        $null=Invoke-Coda 'SymbianInstall' 'getPackageInfo' @(,[string[]]@('e7b31101','e7b31103','e7b31106','e7b31102','e7b31104','e7b31105'))
    } elseif($Action -eq 'probe') {
        $null=Invoke-Coda 'SymbianOSData' 'getRomInfo'
        $null=Invoke-Coda 'SymbianOSData' 'getQtVersion'
    } elseif($Action -eq 'content-info') {
        $r=Invoke-Coda 'FileSystem' 'open' @('C:\data\BelleWall\video-frames.bin',1,@{}); $h=ConvertFrom-Json $r[3]
        try {$r=Invoke-Coda 'FileSystem' 'read' @($h,0,48); $b=[Convert]::FromBase64String((ConvertFrom-Json $r[2])); Write-Host ([BitConverter]::ToString($b))} finally {$null=Invoke-Coda 'FileSystem' 'close' @($h)}
    } elseif($Action -eq 'inventory') {
        foreach($name in @('preparation-pending.bin','candidate-session.bin','wallpaper-rollback.bin','pages-rollback.bin','selected-wallpaper.txt','selected-video.txt','selected-web.txt','video-frames.bin')){
            try {$r=Invoke-Coda 'FileSystem' 'open' @(('C:\data\BelleWall\'+$name),1,@{}); $h=ConvertFrom-Json $r[3]; Write-Host ('INVENTORY present '+$name); $null=Invoke-Coda 'FileSystem' 'close' @($h)} catch {Write-Host ('INVENTORY '+$name+' '+$_.Exception.Message)}
        }
    } elseif($Action -in @('status','status-details','project-processes','terminate','fault-owner','fault-web')) {
        # CODA findRunningProcesses misclassified p796 (bellewall) as belleweb.
        # Enumerate process contexts and validate executable + UID independently.
        $projectUids=@{bellewall='e7b31101';belleweb='e7b31130';belleprobe='e7b31102';bellepaper='e7b31103';bellecache='e7b31104';bellerenderhost='e7b31108'}
        $children=$client.Call('Processes','getChildren',@('""','false'))
        if($children[2]){throw ('Process enumeration failed: '+$children[2])}
        $projectMatches=[System.Collections.Generic.List[object]]::new()
        $gone=0
        foreach($id in (ConvertFrom-Json $children[3])){
            $reply=$client.Call('Processes','getContext',@((ConvertTo-Json -InputObject $id -Compress)))
            if($reply[2]){
                $errorInfo=ConvertFrom-Json $reply[2]
                if($errorInfo.AltCode -eq 4294967295 -or $errorInfo.AltCode -eq -1){$gone++;continue}
                throw ('Process context failed: '+$reply[2])
            }
            $context=ConvertFrom-Json $reply[3]
            if($context.ID -ne $id){throw 'Process context ID mismatch; refusing to classify'}
            foreach($name in $projectUids.Keys){
                if($context.File -ieq ('C:\sys\bin\'+$name+'.exe') -and $context.Name -imatch ('^'+[regex]::Escape($name)+'\.exe\['+$projectUids[$name]+'\]')){
                    $projectMatches.Add([pscustomobject]@{Package=$name;ID=$context.ID;File=$context.File;Name=$context.Name})
                }
            }
        }
        Write-Host ('VERIFIED project processes '+(ConvertTo-Json -InputObject @($projectMatches.ToArray()) -Compress))
        Write-Host ('Enumeration completed; exited during scan='+$gone)
        if($Action -in @('fault-owner','fault-web')){
            if(-not $ContextId -or -not $SessionToken){throw 'Fault injection requires exact context and expected session token'}
            $targetPackage=if($Action -eq 'fault-owner'){'bellepaper'}else{'belleweb'}
            $targets=@($projectMatches | Where-Object {$_.ID -eq $ContextId -and $_.Package -eq $targetPackage})
            if($targets.Count -ne 1){throw 'Exact fault target not uniquely verified'}
            if($Action -eq 'fault-web' -and @($projectMatches | Where-Object {$_.Package -eq 'belleweb'}).Count -ne 1){throw 'Ambiguous web producer'}
            $r=Invoke-Coda 'FileSystem' 'open' @('C:\data\BelleWall\candidate-session.bin',1,@{});$handle=ConvertFrom-Json $r[3]
            try{$r=Invoke-Coda 'FileSystem' 'read' @($handle,0,64);if($r[3]){throw 'Journal read failed'};$bytes=[Convert]::FromBase64String((ConvertFrom-Json $r[2]))}finally{$null=Invoke-Coda 'FileSystem' 'close' @($handle)}
            if($bytes.Length -ne 40){throw 'Wrong journal length'}
            $values=0..9 | ForEach-Object {[BitConverter]::ToUInt32($bytes,$_ * 4)}
            [uint64]$checksum=2166136261
            for($i=0;$i -lt 36;$i++){$checksum=(($checksum -bxor [uint64]$bytes[$i])*16777619) -band 4294967295}
            $token=('{0:x8}{1:x8}' -f $values[2],$values[3])
            if($values[0] -ne 0x31535742 -or $values[1] -ne 2 -or $values[5] -ne 3 -or $values[6] -ne 0 -or $checksum -ne $values[9] -or $token -cne $SessionToken){throw 'Current running continuous session does not match expected token/checksum'}
            if($Action -eq 'fault-owner' -and $ContextId -cne ('p'+$values[4])){throw 'Target is not journal owner; guard must survive'}
            $item=$targets[0];$r=Invoke-Coda 'Processes' 'getContext' @($item.ID);$now=ConvertFrom-Json $r[3]
            if($now.File -cne $item.File -or $now.Name -cne $item.Name){throw 'Target identity changed'}
            Write-Host ('FAULT verified action='+$Action+' target='+$ContextId+' owner='+$values[4]+' token='+$token)
            $null=Invoke-Coda 'Processes' 'terminate' @($item.ID)
        }elseif($Action -eq 'terminate'){
            if($Package -in @('bellepaper','belleweb')){throw 'Use session-checked single-target fault action for coordinator or producer'}

            foreach($item in $projectMatches){if($item.Package -eq $Package){
                # Recheck identity immediately before terminating only this app.
                $r=Invoke-Coda 'Processes' 'getContext' @($item.ID);$now=ConvertFrom-Json $r[3]
                if($now.File -cne $item.File -or $now.Name -cne $item.Name){throw 'Process identity changed; refusing termination'}
                $null=Invoke-Coda 'Processes' 'terminate' @($item.ID)
            }}
        }elseif($Action -ne 'project-processes'){
            Write-Host ('VERIFIED selected '+$Package+' '+(ConvertTo-Json -InputObject @($projectMatches | Where-Object {$_.Package -eq $Package}) -Compress))
            if($ContextId){$null=Invoke-Coda 'Processes' 'getContext' @($ContextId)}
        }
    } elseif($Action -in @('stop','native-stop')) {
        $reply=Invoke-Coda 'FileSystem' 'open' @(('C:\data\BelleWall\'+$(if($Action -eq 'native-stop'){'native-stop'}else{'stop'})),26,@{})
        $handle=ConvertFrom-Json $reply[3]
        $null=Invoke-Coda 'FileSystem' 'close' @($handle)
    } elseif($Action -eq 'start') {
        # One routing table per executable; no generic fallback for new modes.
        $exe=if($Mode -in @('native','native-restore','native-video','native-probe-video','native-web','native-software-video','native-crash-test','native-benchmark','native-benchmark-fast','native-benchmark-size','native-benchmark-limit','native-benchmark-observe','native-screen-probe','native-recover-benchmark','native-wallpaper-state','native-cache-live','native-cache-live-redraw','native-cache-live-speed','native-cache-live-flush','native-cache-live-content','native-cache-live-web','native-cache-live-stream','native-cache-live-pages','native-cache-live-local','native-cache-live-local-content','native-cache-live-local-web','native-cache-live-local-pages','native-page-activate','native-render-plugin-probe','candidate-blocks','candidate-video','candidate-web','candidate-stop','candidate-recover','candidate-video-600','candidate-web-600','candidate-video-continuous','candidate-web-continuous')){'C:\sys\bin\bellepaper.exe'}elseif($Mode -in @('render-host','hs-inspect','make-render-odt','install-render-widget','install-render-widget-v1','remove-render-widget','attach-render-widget','attach-render-pages','detach-render-widget','guard-render-widget-manual','list-render-widget','inspect-render-definition')){'C:\sys\bin\bellerenderhost.exe'}elseif($Mode -in @('snapshot','hs-inventory','hs-manifest')){'C:\sys\bin\belleprobe.exe'}elseif($Mode -eq 'cache-probe'){'C:\sys\bin\bellecache.exe'}elseif($Mode -eq 'inspect-render-repo'){'C:\sys\bin\bellerenderrepo.exe'}elseif($Mode -in @('blocks','legacy-blocks','video','web','stop','diagnose','settings')){'C:\sys\bin\bellewall.exe'}else{throw 'Invalid prototype mode'}
        $arguments=[object[]]@('', $exe, [string[]]$(if($Mode -in @('candidate-video-continuous','candidate-web-continuous')){@(('--'+$Mode.Substring(0,$Mode.Length-11)),'--continuous')}elseif($Mode -in @('candidate-video-600','candidate-web-600')){@(('--'+$Mode.Substring(0,$Mode.Length-4)),'--600')}else{@('--'+$(if($Mode.StartsWith('native-')){$Mode.Substring(7)}else{$Mode}))}), [string[]]@(), $false)
        $null=Invoke-Coda 'Processes' 'start' $arguments
    } elseif($Action -in @('stage-sywp','import-sywp')) {
        $remote='E:\BelleWall-'+$Fixture+'.sywp'
        if($Action -eq 'stage-sywp'){
            $name='examples/'+$Fixture+'.sywp'; $local=[IO.Path]::GetFullPath((Join-Path $Checkpoint $name))
            $validation=Get-Content -LiteralPath (Join-Path $Checkpoint 'validation.json') -Raw | ConvertFrom-Json
            $expected=($validation.files | Where-Object {$_.name -eq $name}).sha256
            if(-not $expected -or (Get-FileHash -LiteralPath $local -Algorithm SHA256).Hash.ToLower() -ne $expected){throw 'SYWP fixture checkpoint hash mismatch'}
            $bytes=[IO.File]::ReadAllBytes($local); if($bytes.Length -gt 8388608){throw 'Device fixture upload limit is 8 MiB'}
            $reply=Invoke-Coda 'FileSystem' 'open' @($remote,26,@{}); $handle=ConvertFrom-Json $reply[3]
            try{for($offset=0;$offset -lt $bytes.Length;$offset+=512){$count=[Math]::Min(512,$bytes.Length-$offset);$null=Invoke-Coda 'FileSystem' 'write' @($handle,$offset,[Convert]::ToBase64String($bytes,$offset,$count))}}finally{$null=Invoke-Coda 'FileSystem' 'close' @($handle)}
            Write-Host ('Staged verified SYWP '+$remote+' bytes='+$bytes.Length)
        }else{
            $null=Invoke-Coda 'Processes' 'start' @('', 'C:\sys\bin\bellewall.exe', [string[]]@('--import-sywp',$remote), [string[]]@(), $false)
        }
    } elseif($Action -in @('install','stage-install')) {
        $sis=[IO.Path]::GetFullPath((Join-Path $Checkpoint ($Package+'-selfsigned.sisx')))
        $validation=Get-Content -LiteralPath (Join-Path $Checkpoint 'validation.json') -Raw | ConvertFrom-Json
        $expected=($validation.files | Where-Object {$_.name -eq ($Package+'-selfsigned.sisx')}).sha256
        if(-not $expected -or (Get-FileHash -LiteralPath $sis -Algorithm SHA256).Hash.ToLower() -ne $expected){throw 'Checkpoint SIS hash mismatch'}
        $bytes=[IO.File]::ReadAllBytes($sis)
        $remote=if($Action -eq 'stage-install'){'C:\data\BelleWall\'+$Package+'-manual.sisx'}else{'C:\data\BelleWall-update.sis'}
        $reply=Invoke-Coda 'FileSystem' 'open' @($remote,26,@{})
        $handle=ConvertFrom-Json $reply[3]
        try {
            for($offset=0;$offset -lt $bytes.Length;$offset+=512){
                $count=[Math]::Min(512,$bytes.Length-$offset)
                $null=Invoke-Coda 'FileSystem' 'write' @($handle,$offset,[Convert]::ToBase64String($bytes,$offset,$count))
            }
        } finally {$null=Invoke-Coda 'FileSystem' 'close' @($handle)}
        Write-Host ('Uploaded SIS bytes='+$bytes.Length)
        if($Action -eq 'stage-install'){Write-Host ('Ready for manual installation: '+$remote)}else{$null=Invoke-Coda 'SymbianInstall' 'install' @($remote,'C')}
    } else {
        $remote=switch($Action){'pull-library' {if(-not $LibraryFile){throw 'Specify a hashed library filename'};'C:\data\BelleWall\library\'+$LibraryFile};'pull-diagnostics' {'E:\BelleWall-diagnostics.txt'};'candidate-log' {'C:\data\BelleWall\candidate-metrics.csv'};'pull-project' {if($ProjectFile.EndsWith('.exe')){'C:\sys\bin\'+$ProjectFile}else{'C:\data\BelleWall\'+$ProjectFile}};'pull-render-odt' {'C:\data\BelleWall\render-widget\bellewall.o0000'};'pull-longrun-odt' {'C:\data\BelleWall\render-longrun-widget\bellewall.o0000'};'pull-rom' {'Z:\sys\bin\'+$Module};'pull-snapshot' {'C:\data\BelleWall\snapshot\'+$Module};'cache-log' {'C:\data\BelleWall\cache-probe.log'};'render-log' {'C:\data\BelleWall\render-plugin.log'};'probe-log' {'C:\data\BelleWall\probe.log'};'pull-screen' {'C:\data\BelleWall\native-frame-9000.bmp'};'bench-log' {'C:\data\BelleWall\wallpaper-benchmark.csv'};'native-log' {'C:\data\BelleWall\native-wallpaper.log'};default {'C:\data\BelleWall\host.log'}}
        $reply=Invoke-Coda 'FileSystem' 'open' @($remote,1,@{})
        if($reply[2]){throw ('File open failed: '+$reply[2])}
        $handle=ConvertFrom-Json $reply[3]
        $stream=[IO.MemoryStream]::new()
        try {
            $eof=$false
            for($offset=0;$offset -lt 16777216;){
                $reply=Invoke-Coda 'FileSystem' 'read' @($handle,$offset,4096)
                if($reply[3]){throw ('File read failed: '+$reply[3])}
                $data=[Convert]::FromBase64String((ConvertFrom-Json $reply[2]))
                $stream.Write($data,0,$data.Length);$offset+=$data.Length
                if(($reply[4] -eq 'true') -or $data.Length -eq 0){$eof=$true;break}
            }
            if(-not $eof){throw 'Read limit exceeded; partial file not saved'}
            [IO.File]::WriteAllBytes([IO.Path]::GetFullPath($OutputFile),$stream.ToArray())
        } finally { $null=Invoke-Coda 'FileSystem' 'close' @($handle);$stream.Dispose() }
    }
} finally {$client.Dispose()}


