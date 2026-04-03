$filePath = "e:\Mafia\Game.cpp"
$content = Get-Content $filePath -Raw

# Add sf::Event declaration at the start of handleEvents
$handlerStart = $content.IndexOf("void Game::handleEvents()")
$bracePos = $content.IndexOf("{", $handlerStart)
$contentStart = $bracePos + 1
$firstInsertPos = $content.IndexOf("mouseClicked = false;", $contentStart)

# Prepare the event polling fixes
$oldEventLoop = @"
    // Block input during transitions
    if (transitioning) {
        while (const auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) { window.close(); return; }
        }
        return;
    }

    while (const auto ev = window.pollEvent()) {

        if (ev->is<sf::Event::Closed>()) { window.close(); return; }

        if (const auto* rs = ev->getIf<sf::Event::Resized>()) {
            (void)rs;
            updateGameView();
        }

        // ── Left click ──────────────────────────────────────────────────────
        if (const auto* mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
            if (mb->button == sf::Mouse::Button::Left) {
                clickPos     = window.mapPixelToCoords(mb->position);
                mouseClicked = true;
            }
        }

        // ── Text input (lobby name field + discussion chat) ─────────────────
        if (const auto* te = ev->getIf<sf::Event::TextEntered>()) {
            auto ch = te->unicode;
"@

$newEventLoop = @"
    // Block input during transitions
    sf::Event event;
    if (transitioning) {
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) { window.close(); return; }
        }
        return;
    }

    while (window.pollEvent(event)) {

        if (event.type == sf::Event::Closed) { window.close(); return; }

        if (event.type == sf::Event::Resized) {
            updateGameView();
        }

        // ── Left click ──────────────────────────────────────────────────────
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                clickPos     = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                mouseClicked = true;
            }
        }

        // ── Text input (lobby name field + discussion chat) ─────────────────
        if (event.type == sf::Event::TextEntered) {
            auto ch = event.text.unicode;
"@

# Also need to fix more event patterns
$oldKeyPressed = @"
        // ── Key pressed ─────────────────────────────────────────────────────
        if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
            if (kp->code == sf::Keyboard::Key::F1) {
"@

$newKeyPressed = @"
        // ── Key pressed ─────────────────────────────────────────────────────
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::F1) {
"@

$content = $content.Replace($oldEventLoop, $newEventLoop)
$content = $content.Replace($oldKeyPressed, $newKeyPressed)

# Replace all the key code references
$keyCodeReplacements = @(
    ("kp->code == sf::Keyboard::Key::F9", "event.key.code == sf::Keyboard::F9"),
    ("kp->code == sf::Keyboard::Key::Escape", "event.key.code == sf::Keyboard::Escape"),
    ("kp->code == sf::Keyboard::Key::Tab", "event.key.code == sf::Keyboard::Tab"),
    ("kp->code == sf::Keyboard::Key::W", "event.key.code == sf::Keyboard::W"),
    ("kp->code == sf::Keyboard::Key::G", "event.key.code == sf::Keyboard::G"),
    ("kp->code == sf::Keyboard::Key::Enter", "event.key.code == sf::Keyboard::Return"),
    ("kp->code == sf::Keyboard::Key::M", "event.key.code == sf::Keyboard::M"),
)

foreach ($pair in $keyCodeReplacements) {
    $content = $content.Replace($pair[0], $pair[1])
}

Set-Content $filePath $content

Write-Host "SFML event handling fixed!"
