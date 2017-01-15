<h1>Komunikator IRC</h1>

---

<h2>Protokół komunikacyjny</h2>

<h3>Wysyłanie wiadomości:</h3>
- pierwsza wiadomość zawierająca wielkość wysyłanej wiadomości
- druga wiadomość zawierająca tekst wiadomości

<h3>Kodowanie wiadomości:</h3>
- <code>#cmd#arg1#arg2</code>

Wysyłanie tekstu:
- <code>#send#room_number#text</code>

Logowanie:
- <code>#login#nickname</code>
- serwer zwraca status wykonania

Wylogowanie:
- <code>#logout</code>

Dołączenie do pokoju:
- <code>#join#room_number</code>
- serwer zwraca status wykonania

Opuszczenie pokoju:
- <code>#leave#room_number</code>
- serwer zwraca status wykonania


<h3>Odpowiedź serwera:</h3>

Połączenie z serwerem:
- OK    <code>#success#connect</code>
- ERROR <code>#error#connect</code>

Dołączenie do pokoju:
- OK    <code>#success#join#room_number</code>
- ERROR <code>#error#join#room_number</code>

Opuszczenie pokoju:
- OK    <code>#success#leave#room_number</code>
- ERROR <code>#error#join#room_number</code>

Logowanie:
- OK    <code>#success#login#nickname</code>
- ERROR <code>#error#join#room_number</code>

Lista użytkowników (wysyłana przy każdej zmianie stanu liczby użytkowników):
- <code>#users#room_number#nick1#nick2#...</code>

---
