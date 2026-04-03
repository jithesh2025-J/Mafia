# Comprehensive SFML 2.x compatibility fixes
$file = 'e:\Mafia\Game.cpp'
$content = [System.IO.File]::ReadAllText($file)

Write-Host "Starting comprehensive SFML 2.x fixes..."

# Fix remaining old SFML 3.x event patterns
$content = $content -replace 'if \(const auto\* rs = ev->getIf<sf::Event::Resized>\(\)', 'if (event.type == sf::Event::Resized)'
$content = $content -replace '\(void\)rs;', ''

# Fix remaining old event patterns  
$content = $content -replace 'if \(const auto\* mb = ev->getIf<sf::Event::MouseButtonPressed>\(\)', 'if (event.type == sf::Event::MouseButtonPressed)'
$content = $content -replace 'if \(mb->button == sf::Mouse::Button::Left\)', 'if (event.mouseButton.button == sf::Mouse::Left)'
$content = $content -replace 'window\.mapPixelToCoords\(mb->position\)', 'window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))'

# Fix remaining text input event patterns
$content = $content -replace 'if \(const auto\* te = ev->getIf<sf::Event::TextEntered>\(\)', 'if (event.type == sf::Event::TextEntered)'
$content = $content -replace 'if \(const auto\* kp = ev->getIf<sf::Event::KeyPressed>\(\)', 'if (event.type == sf::Event::KeyPressed)'
$content = $content -replace 'auto ch = te->unicode', 'auto ch = event.text.unicode'

# Fix sf::Text constructor calls - convert from (font, str, size) to (str, font, size)
$content = $content -replace 'sf::Text (\w+)\(font, "([^"]+)", (\d+)\);', 'sf::Text $1("$2", font, $3);'
$content = $content -replace 'sf::Text (\w+)\(font, ([a-zA-Z_]\w*), (\d+)\);', 'sf::Text $1($2, font, $3);'
$content = $content -replace 'sf::Text\(font, (std::string\([^)]+\)), (\d+)\)', 'sf::Text($1, font, $2)'

# Fix all .size.x and .size.y to .width and .height
$content = $content -replace '\.size\.x', '.width'
$content = $content -replace '\.size\.y', '.height'

# Fix all .position.x and .position.y to .left and .top
$content = $content -replace '\.position\.x', '.left'
$content = $content -replace '\.position\.y', '.top'

# Fix setPosition() calls that had .position
$content = $content -replace 'setPosition\(([^,]+)\.position\)', 'setPosition($1.left, $1.top)'

# Fix remaining .size references (whole)
$content = $content -replace '(\b[a-zA-Z_]\w*)\.size\)', '$1.width), ($1.height)'

[System.IO.File]::WriteAllText($file, $content)
Write-Host "SFML compatibility fixes applied successfully!"
