<h1>Komunikator internetowy typu IRC</h1>

---
<h2>Serwer</h2>
- Wykorzystuje synchronizacje I/O multiplexing z funkcją select
- Port ustawiony domyślnie na 3012 (numer portu można zmienić wewnątrz funkcji initSockAddr)
- Kompilacja:
    - <code>gcc -Wall main.c -o server</code>
- Uruchomienie:
    - <code>./server</code>
    
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
- ERROR <code>#error#leave#room_number</code>

Logowanie:
- OK    <code>#success#login#nickname</code>
- ERROR <code>#error#login#nickname</code>

Lista użytkowników (wysyłana przy każdej zmianie stanu liczby użytkowników):
- <code>#users#room_number#nick1#nick2#...</code>

---

<h2>Aplikacja klienta</h2>
Aplikacja wykonana w JavaFX.

Projekt przygotowany w środowisku IntelliJ. Aby skompilować go w innym środowisku 
należy stworzyć nowy projekt aplikacji JavaFX i wykorzystać paczkę 
<code>src</code> z projektu <code>javaClient</code>. 

Alternatywą jest wywołanie:

<code>javac Main.java</code>

<code>java Main</code>

Aplikację można również uruchomić z <code>javaClient.jar</code>.

<h3>Logowanie:</h3>
- Użytkownik wpisuje numer portu oraz adres IP serwera (Port jest domyślnie ustawiony na taki jak w serwerze - 3012).
- Użytkownik wpisuje pożądany login.
- Główne okno aplikacji uruchamiane jest po kliknięciu przycisku "Login" jeżeli:
    - Nie wystąpił błąd łączenia z serwerem (np. błędny numer portu lub adres IP),
    - Login nie jest pusty ani dłuższy niż 15 znaków,
    - Login nie jest zarezerwowany ("system"),
    - Login nie jesti zajęty przez innego użytkownika.

<h3>Chat:</h3>
- Użytkownik automatycznie dołącza do pokoju numer 0 - znajdują się w nim
wszyscy użytkownicy. Pokoju 0 nie można opuścić.
- Znak (!) przy numerze pokoju oznacza, że użytkownik dołączył do danego pokoju.
- Użytkownik może dołączyć do pokoju po wybraniu pożądanego numeru na liście 
pokoi oraz kliknięciu przycisku "Join". Jeżeli użytkownik jest już w danym 
pokoju w oknie chatu wyświetlni się odpowiedni komunikat o błędzie.
- Użytkownik może opuścić pokój po wybraniu pożadanego numeru na liście pokoi 
oraz kliknięciu przycisku "Leave". Jeżeli użytkownik nie dołączył wcześniej do 
danego pokoju wyświetlni się odpowiedni komunikat o błędzie.
- Użytkownik widzi wiadomości z pokoju aktualnie wybranego na liście pokoi w 
formacie <code>nick: wiadomość</code>.
- Użytkownik widzi listę użytkowników znajdujących się w pokoju aktualnie 
wybranym na liście pokoi.
- Użytkownik może napisać wiadomość do pokoju aktualnie wybranego na liście 
pokoi poprzez wpisanie tesktu w polu wpisywania wiadomości oraz naciśnięciu 
klawisza "Enter".
- Znak (*) przy numerze pokoju oznacza, że w danym pojawiła się nowa wiadomość.