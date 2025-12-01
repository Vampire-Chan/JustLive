# AutoPush.ps1
# Checks for changes every minute and pushes to git with advanced, context-aware commit messages.
# Requires Git credentials to be cached/configured.

$IntervalSeconds = 60

Write-Host "Starting AutoPush script... (Ctrl+C to stop)" -ForegroundColor Cyan
Write-Host "Ensure you have authenticated with GitHub manually once!" -ForegroundColor Gray

# List of "human-like" verbs to randomize commit messages
$Verbs = @(
    "Updated", "Refactored", "Tweaked", "Modified", "Polished",
    "Enhanced", "Cleanup in", "Adjusted", "Improved", "Optimized", "Reviewed"
)
$BroadVerbs = @(
    "Project Update:", "General improvements:", "WIP changes:",
    "Routine sync:", "Continuous development:", "Major refactor:"
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
        # Status format: " M path/to/file.ext" or "?? path/to/newfile.ext"
        $ModifiedFiles = ($ModifiedStatus -split "`n") | ForEach-Object { 
            # Remove status part (first 3 chars) and trim whitespace
            $_.Substring(3).Trim() 
        }

        $CommitMessage = ""

        if ($ModifiedFiles.Count -eq 0) {
            # This should ideally not happen if $ModifiedStatus is not empty, but safety check
            $CommitMessage = "$($BroadVerbs | Get-Random) Miscellaneous changes ($Time)"
        } elseif ($ModifiedFiles.Count -eq 1) {
            # Single file changed
            $FilePath = $ModifiedFiles[0]
            $FileName = Split-Path $FilePath -Leaf
            $ParentDir = Split-Path $FilePath -Parent
            $ParentDirName = Split-Path $ParentDir -Leaf
            if ([string]::IsNullOrEmpty($ParentDirName)) {
                $ParentDirName = "Root"
            }
            $CommitMessage = "$($Verbs | Get-Random) $FileName in $ParentDirName ($Time)"
        } else {
            # Multiple files changed
            $TopLevelContexts = New-Object System.Collections.Generic.HashSet[string]
            $TotalFileCount = $ModifiedFiles.Count

            foreach ($File in $ModifiedFiles) {
                $Parts = $File -split "[/\]"
                if ($Parts.Length -gt 0) {
                    $Context = ""
                    if ($Parts[0] -eq "Source" -and $Parts.Length -gt 2) {
                        # Source/JustLive/Core/... -> Core
                        # Source/JustLive/Managers/Audio/... -> Managers/Audio
                        if ($Parts.Length -gt 3 -and $Parts[2] -eq "Managers") {
                            $Context = $Parts[2] + "/" + $Parts[3] # e.g. Managers/Audio
                        } elseif ($Parts.Length -gt 2) {
                            $Context = $Parts[2] # e.g. Core, Gameplay
                        }
                    } elseif ($Parts[0] -eq "Plugins" -and $Parts.Length -gt 1) {
                        # Plugins/Scripting -> Scripting
                        $Context = $Parts[1]
                    } elseif ($Parts[0] -eq "Tools" -and $Parts.Length -gt 1) {
                        # Tools/StandaloneScriptCompiler -> StandaloneScriptCompiler
                        $Context = "Tools/" + $Parts[1]
                    } elseif ($Parts[0] -ne "") {
                        $Context = $Parts[0] # Config, Content, etc.
                    }
                    if ($Context -ne "") {
                        $TopLevelContexts.Add($Context)
                    }
                }
            }
            
            # Decide on message based on spread
            if ($TopLevelContexts.Count -eq 0) {
                 $CommitMessage = "$($BroadVerbs | Get-Random) General project files ($TotalFileCount files) ($Time)"
            } elseif ($TopLevelContexts.Count -eq 1) {
                $ComponentName = ($TopLevelContexts | Sort-Object)[0]
                $CommitMessage = "$($Verbs | Get-Random) $ComponentName ($TotalFileCount files) ($Time)"
            } elseif ($TopLevelContexts.Count -lt 5 -and $TotalFileCount -lt 20) {
                # Scattered but not too many
                $ComponentNames = ($TopLevelContexts | Sort-Object) -join ", "
                $CommitMessage = "$($Verbs | Get-Random) $ComponentNames ($TotalFileCount files) ($Time)"
            } else {
                # Broad/Many changes
                $CommitMessage = "$($BroadVerbs | Get-Random) Project-wide changes ($TotalFileCount files across $($TopLevelContexts.Count) areas) ($Time)"
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
