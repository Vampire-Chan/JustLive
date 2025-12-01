# AutoPush.ps1
# Checks for changes every minute and pushes to git
# Requires Git credentials to be cached/configured.

$IntervalSeconds = 60

Write-Host "Starting AutoPush script... (Ctrl+C to stop)" -ForegroundColor Cyan
Write-Host "Ensure you have authenticated with GitHub manually once!" -ForegroundColor Gray

while ($true) {
    # Check for changes (staged or unstaged)
    $Status = git status --porcelain
    
    if ($Status) {
        $Time = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Write-Host "[$Time] Changes detected. Committing..." -ForegroundColor Yellow
        
        # Stage all changes
        git add .
        
        # Commit
        $CommitMsg = "Auto-save: $Time"
        git commit -m "$CommitMsg" | Out-Null
        
        Write-Host "[$Time] Pushing to remote..." -ForegroundColor Yellow
        
        # Push (redirecting stderr to stdout to capture errors)
        $PushOutput = git push 2>&1
        
        if ($LASTEXITCODE -eq 0) {
             Write-Host "[$Time] Successfully pushed." -ForegroundColor Green
        } else {
             Write-Host "[$Time] Push failed!" -ForegroundColor Red
             # Check for common auth errors
             if ($PushOutput -match "Authentication failed" -or $PushOutput -match "could not read Username") {
                 Write-Host "Authentication Error: Please run 'git push' manually to sign in." -ForegroundColor Red
             } else {
                 Write-Host $PushOutput -ForegroundColor Red
             }
        }
    } else {
        # Optional: Heartbeat
        # Write-Host "." -NoNewline
    }
    
    Start-Sleep -Seconds $IntervalSeconds
}