# Final comprehensive SFML 2.x Text constructor fixes
$file = 'e:\Mafia\Game.cpp'
$content = [System.IO.File]::ReadAllText($file)

Write-Host "Applying final SFML 2.x Text constructor fixes..."

# Fix safeR.size issue
$content = $content -replace 'sf::Vector2f btnSize = safeR\.size;', 'sf::Vector2f btnSize(safeR.width, safeR.height);'

# Rather than complex regex, do direct replacements for specific patterns
# First, backup and use a temporary file to make the edits

# Most common patterns that weren't caught:
# sf::Text name(font, stringExpr, sizeNumber) -> sf::Text name(stringExpr, font, sizeNumber)

$lines = $content -split "`n"
$newLines = @()

foreach ($line in $lines) {
    # Check if line contains sf::Text constructor with old order
    if ($line -match 'sf::Text\s+(\w+)\s*\(\s*font\s*,' -and $line -notmatch 'sf::Text\s+(\w+)\s*\([^,]+,\s*font') {
        # This line has old constructor order, we need to fix it
        # Pattern: sf::Text NAME(font, EXPR, NUMBER);
        
        # Try different patterns
        if ($line -match 'sf::Text\s+(\w+)\s*\(\s*font\s*,\s*"([^"]*)"\s*,\s*(\d+)\s*\)') {
            # String literal version
            $name = $matches[1]
            $str = $matches[2]
            $size = $matches[3]
            $line = $line -replace [regex]::Escape($matches[0]), "sf::Text $name(`"$str`", font, $size)"
        }
        elseif ($line -match 'sf::Text\s+(\w+)\s*\(\s*font\s*,\s*([a-zA-Z_]\w*)\s*,\s*(\d+)\s*\)') {
            # Variable version
            $name = $matches[1]
            $var = $matches[2]
            $size = $matches[3]
            $line = $line -replace [regex]::Escape($matches[0]), "sf::Text $name($var, font, $size)"
        }
        elseif ($line -match 'sf::Text\s+(\w+)\s*\(\s*font\s*,\s*(.*?)\s*,\s*(\w+)\s*\)') {
            # Generic expression version (must be careful to not match too much)
            $name = $matches[1]
            $epr = $matches[2].Trim()
            $size = $matches[3]
            
            # Only fix if expression doesn't already have 'font' in it
            if ($expr -notmatch '\bfont\b') {
                $line = $line -replace [regex]::Escape($matches[0]), "sf::Text $name($expr, font, $size)"
            }
        }
    }
    
    $newLines += $line
}

$newContent = $newLines -join "`n"

[System.IO.File]::WriteAllText($file, $newContent)
Write-Host "Final SFML fixes applied!"
