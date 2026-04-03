@echo off
REM This script will systematically fix all SFML 2.x compatibility issues

echo Fixing SFML 2.x compatibility issues...

cd /d e:\Mafia

REM Use sed-like replacements to fix remaining issues
REM We'll do a multi-pass approach

powershell -Command "
\$file = 'e:\\Mafia\\Game.cpp'
\$content = [System.IO.File]::ReadAllText(\$file)

REM Fix all remaining old SFML 3.x event patterns
\$content = \$content -replace 'if \(const auto\* rs = ev->getIf<sf::Event::Resized>\(\)', 'if (event.type == sf::Event::Resized)'
\$content = \$content -replace '\(void\)rs;', ''

REM  Fix remaining old event patterns
\$content = \$content -replace 'if \(const auto\* mb = ev->getIf<sf::Event::MouseButtonPressed>\(\)', 'if (event.type == sf::Event::MouseButtonPressed)'
\$content = \$content -replace 'if \(mb->button == sf::Mouse::Button::Left\)', 'if (event.mouseButton.button == sf::Mouse::Left)'
\$content = \$content -replace 'window\.mapPixelToCoords\(mb->position\)', 'window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))'

REM Fix remaining old key patterns
\$content = \$content -replace 'if \(const auto\* te = ev->getIf<sf::Event::TextEntered>\(\)', 'if (event.type == sf::Event::TextEntered)'
\$content = \$content -replace 'if \(const auto\* kp = ev->getIf<sf::Event::KeyPressed>\(\)', 'if (event.type == sf::Event::KeyPressed)'
\$content = \$content -replace 'auto ch = te->unicode', 'auto ch = event.text.unicode'

REM Fix all sf::Text constructor calls to SFML 2.x style
REM Pattern: sf::Text oldname(font, string, size)  -> sf::Text newname(string, font, size)
\$content = \$content -replace 'sf::Text (\w+)\(font, ([^,]+), (\d+)\)', 'sf::Text \$1(\$2, font, \$3)'

REM Fix all .size.x and .size.y to .width and .height
\$content = \$content -replace '\.size\.x', '.width'
\$content = \$content -replace '\.size\.y', '.height'

REM Fix all .position.x and .position.y to .left and .top
\$content = \$content -replace '\.position\.x', '.left'
\$content = \$content -replace '\.position\.y', '.top'

REM Fix .position (direct) to setPosition
\$content = \$content -replace 'setPosition\(([^,]+)\.position\)', 'setPosition(\$1.left, \$1.top)'

[System.IO.File]::WriteAllText(\$file, \$content)
Write-Host 'SFML compatibility fixes applied!'
"

echo Done!
pause
