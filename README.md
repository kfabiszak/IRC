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
- serwer zwraca <code>#success#</code>

Wylogowanie:
- <code>#logout</code>

Dołączanie do pokoju:
- <code>#join#room_number</code>
- serwer zwraca <code>#success#</code>

Opuszczenie pokoju:
- <code>#leave#room_number</code>


<h3>Odpowiedź serwera:</h3>

Poprawnie przetworzenie prośby:
- <code>#success#</code>

Lista użytkowników wysyłana przy każdej zmianie stanu liczby użytkowników:
- <code>#users#room_number#nick1#nick2#...</code>

---
