# AutoPush.ps1
# Checks for changes every minute and pushes to git with advanced, context-aware commit messages.
# Requires Git credentials to be cached/configured.

$IntervalSeconds = 60

Write-Host "Starting AutoPush script... (Ctrl+C to stop)" -ForegroundColor Cyan
Write-Host "Ensure you have authenticated with GitHub manually once!" -ForegroundColor Gray

# List of "human-like" verbs to randomize commit messages
$Verbs = @(
    "Updated", "Refactored", "Tweaked", "Modified", "Polished",
    "Enhanced", "Cleanup in", "Adjusted", "Improved", "Optimized"
)
$BroadVerbs = @(
    "Project Update:", "General improvements:", "WIP changes:",
    "Routine sync:", "Continuous development:"
)
$ActionVerbs = @(
    "Added", "Fixed", "Implemented", "Removed", "Refactored"
)

while ($true) {
    # Get status of changed files, excluding untracked ones (as they should be added manually if intended)
    $ModifiedStatus = git status --porcelain -uno
    
    if ($ModifiedStatus) {
        $Time = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Write-Host "[$Time] Changes detected. Analyzing context for commit message..." -ForegroundColor Yellow
        
        # Parse modified files (A, M, D status)
        $ModifiedFiles = ($ModifiedStatus -split "`n") | ForEach-Object { $_.Substring(3).Trim() }

        $CommitMessage = ""

        if ($ModifiedFiles.Count -eq 0) {
            # This should ideally not happen if $ModifiedStatus is not empty, but safety check
            $CommitMessage = "$($BroadVerbs | Get-Random) Miscellaneous changes ($Time)"
        } elseif ($ModifiedFiles.Count -eq 1) {
            # Single file changed
            $FilePath = $ModifiedFiles[0]
            $FileName = Split-Path $FilePath -Leaf
            $DirName = Split-Path $FilePath -Parent -Leaf
            $CommitMessage = "$($Verbs | Get-Random) $FileName in $DirName ($Time)"
        } else {
            # Multiple files changed
            $TopFolders = New-Object System.Collections.Generic.HashSet[string]
            $TopLevelComponents = New-Object System.Collections.Generic.HashSet[string]

            foreach ($File in $ModifiedFiles) {
                $Parts = $File -split "[/\\]" 
                if ($Parts.Length -gt 0) {
                    $TopFolders.Add($Parts[0]) # e.g., Source, Plugins, Config
                    if ($Parts[0] -eq "Source" -and $Parts.Length -gt 2) {
                        # Source/JustLive/Core/... -> Core
                        $TopLevelComponents.Add($Parts[2]) 
                    } elseif ($Parts[0] -eq "Plugins" -and $Parts.Length -gt 1) {
                        # Plugins/Scripting/... -> Scripting
                        $TopLevelComponents.Add($Parts[1])
                    } elseif ($Parts[0] -eq "Tools" -and $Parts.Length -gt 1) {
                        # Tools/StandaloneScriptCompiler -> StandaloneScriptCompiler
                        $TopLevelComponents.Add($Parts[1])
                    } else {
                        $TopLevelComponents.Add($Parts[0]) # e.g., Config, Content
                    }
                }
            }

            if ($TopLevelComponents.Count -le 2 -and $ModifiedFiles.Count -lt 10) {
                # Focused changes in one or two components
                $ComponentNames = ($TopLevelComponents | Sort-Object) -join ", "
                $CommitMessage = "$($Verbs | Get-Random) $ComponentNames ($ModifiedFiles.Count files) ($Time)"
            } else {
                # Widespread changes
                $CommitMessage = "$($BroadVerbs | Get-Random) Broad update across $($TopLevelComponents.Count) components ($ModifiedFiles.Count files) ($Time)"
            }
        }
        
        Write-Host "  Commit Message: $CommitMessage" -ForegroundColor Gray
        
        # Stage all changes
        git add .
        
        # Commit
        git commit -m "$CommitMessage" | Out-Null
        
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