$file = 'e:\Mafia\Game.cpp'
$content = [System.IO.File]::ReadAllText($file)

# Simple approach: replace all occurrences of sf::Text(...) patterns
# where font is the first argument and move it to second

# Strategy: Find "sf::Text VARNAME(font," pattern and extract the rest,
# then rebuild as "sf::Text VARNAME(EXPR, font, ...)"

# This is complex with regex, so let's use multiple simpler passes

Write-Host "Pass 1: Fixing sf::Text with concatenations..."
# sf::Text name(font, std::string(...) + something, number)
$content = $content -replace 'sf::Text\s+(\w+)\s*\(\s*font\s*,\s*(std::string\([^)]+\)\s*\+[^,]+)\s*,\s*(\d+)\s*\)', 'sf::Text $1($2, font, $3)'

Write-Host "Pass 2: Fixing sf::Text with function calls..."
# sf::Text name(font, FUNC(...), number)
$content = $content -replace 'sf::Text\s+(\w+)\s*\(\s*font\s*,\s*(\w+\([^)]+\))\s*,\s*(\d+)\s*\)', 'sf::Text $1($2, font, $3)'

Write-Host "Pass 3: Fixing sf::Text with wrapped text calls..."  
# sf::Text name(font, wrapTextToWidth(...), number)
$content = $content -replace 'sf::Text\s+(\w+)\s*\(\s*font\s*,\s*(wrapTextToWidth\([^,]+,[^,]+,[^,]+,[^)]+\))\s*,\s*(\d+)\s*\)', 'sf::Text $1($2, font, $3)'

Write-Host "Pass 4: Fixing remaining sf::Text simple cases..."
# sf::Text name(font, VARNAME, number)  - but avoid if already has font in second pos
$lines = $content -split "`n"
$output = @()
$inMultiLineText = $false
$multiLineBuffer = ""

foreach ($line in $lines) {
    # Check for multi-line sf::Text
    if ($line -match 'sf::Text\s+(\w+)\s*\(\s*font\s*,') {
        # Check if it's a complete single line
        $openParens = ($line | Select-String -Pattern '\(' -AllMatches).Matches.Count
        $closeParens = ($line | Select-String -Pattern '\)' -AllMatches).Matches.Count
        
        if ($openParens -eq $closeParens) {
            # Single line, try a simple fix
            # Replace "font," with the content after comma, then move it
            if ($line -match 'sf::Text\s+(\w+)\s*\(\s*font\s*,\s*(\S+?)\s*,\s*(\S+?)\s*\)') {
                $name = $matches[1]
                $str = $matches[2]
                $size = $matches[3]
                $line = $line -replace [regex]::Escape($matches[0]), "sf::Text $name($str, font, $size)"
            }
        } else {
            $inMultiLineText = $true
            $multiLineBuffer = $line
            continue
        }
    }
    elseif ($inMultiLineText) {
        $multiLineBuffer += "`n" + $line
        $openParens = ($multiLineBuffer | Select-String -Pattern '\(' -AllMatches).Matches.Count
        $closeParens = ($multiLineBuffer | Select-String -Pattern '\)' -AllMatches).Matches.Count
        
        if ($openParens -eq $closeParens) {
            # Found complete multi-line statement
            # For now, just keep as is - this is too complex to fix safely
            $line = $multiLineBuffer
            $inMultiLineText = $false
            $multiLineBuffer = ""
        } else {
            continue
        }
    }
    
    $output += $line
}

$content = $output -join "`n"

# Fix the one safeR.size issue
$content = $content -replace 'sf::Vector2f btnSize = safeR\.size;', 'sf::Vector2f btnSize(safeR.width, safeR.height);'

[System.IO.File]::WriteAllText($file, $content)
Write-Host "Completed!"
