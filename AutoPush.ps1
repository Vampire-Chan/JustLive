# AutoPush.ps1
# Checks for changes every minute and pushes to git

$IntervalSeconds = 60

Write-Host "Starting AutoPush script... (Ctrl+C to stop)" -ForegroundColor Cyan

while ($true) {
    $Status = git status --porcelain
    if ($Status) {
        $Time = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Write-Host "[$Time] Changes detected. Committing and pushing..." -ForegroundColor Yellow
        
        git add .
        git commit -m "Auto-save: $Time"
        
        # Capturing output to avoid cluttering the console if there are network issues, 
        # but printing errors if it fails.
        $PushOutput = git push 2>&1
        if ($LASTEXITCODE -eq 0) {
             Write-Host "[$Time] Successfully pushed." -ForegroundColor Green
        } else {
             Write-Host "[$Time] Push failed:" -ForegroundColor Red
             Write-Host $PushOutput -ForegroundColor Red
        }
    } else {
        # Optional: Uncomment to see heartbeat
        # Write-Host "No changes detected." -ForegroundColor Gray
    }
    
    Start-Sleep -Seconds $IntervalSeconds
}
