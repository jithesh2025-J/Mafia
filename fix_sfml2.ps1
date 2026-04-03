$filePath = "e:\Mafia\Game.cpp"
$content = Get-Content $filePath -Raw

# Fix these patterns one by one
$content = $content.Replace('while (const auto ev = window.pollEvent())', 'sf::Event event; while (window.pollEvent(event))')
$content = $content.Replace('if (ev->is<sf::Event::Closed>())', 'if (event.type == sf::Event::Closed)')
$content = $content.Replace('if (const auto* rs = ev->getIf<sf::Event::Resized>()) { (void)rs;', 'if (event.type == sf::Event::Resized)')
$content = $content.Replace('if (const auto* mb = ev->getIf<sf::Event::MouseButtonPressed>())', 'if (event.type == sf::Event::MouseButtonPressed)')
$content = $content.Replace('if (mb->button == sf::Mouse::Button::Left)', 'if (event.mouseButton.button == sf::Mouse::Left)')
$content = $content.Replace('window.mapPixelToCoords(mb->position)', 'window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y))')
$content = $content.Replace('if (const auto* te = ev->getIf<sf::Event::TextEntered>())', 'if (event.type == sf::Event::TextEntered)')
$content = $content.Replace('auto ch = te->unicode', 'auto ch = event.text.unicode')
$content = $content.Replace('if (const auto* kp = ev->getIf<sf::Event::KeyPressed>())', 'if (event.type == sf::Event::KeyPressed)')
$content = $content.Replace('kp->code == sf::Keyboard::Key::F1', 'event.key.code == sf::Keyboard::F1')
$content = $content.Replace('kp->code == sf::Keyboard::Key::F9', 'event.key.code == sf::Keyboard::F9')
$content = $content.Replace('kp->code == sf::Keyboard::Key::Escape', 'event.key.code == sf::Keyboard::Escape')
$content = $content.Replace('kp->code == sf::Keyboard::Key::Tab', 'event.key.code == sf::Keyboard::Tab')
$content = $content.Replace('kp->code == sf::Keyboard::Key::W', 'event.key.code == sf::Keyboard::W')
$content = $content.Replace('kp->code == sf::Keyboard::Key::G', 'event.key.code == sf::Keyboard::G')
$content = $content.Replace('kp->code == sf::Keyboard::Key::Enter', 'event.key.code == sf::Keyboard::Return')
$content = $content.Replace('kp->code == sf::Keyboard::Key::M', 'event.key.code == sf::Keyboard::M')

Set-Content $filePath $content
Write-Host "Event handling patterns fixed!"
