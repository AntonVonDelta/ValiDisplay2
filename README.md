# ValiDisplay
Cod Arduino si schema electronica pentru un afisaj anod-comun.

Afisajul se "revarsa" peste 9999 inapoi la 0 prin incrementare si de la 0 catre 9999 prin decrementare.

Schimbarea de la `anod comun` la `catod comun` se face in cod prin schimbarea variabilei `display` si in circuit prin inversare colectorilor cu emiterii in cazul tranzistorilor, adaugarea cate unei rezistente mai mici de 2K pe baza fiecarui tranzistor si schimbarea sursei "comune" de 5V cu cea de 0V (minusul).

Circuitul functioneaza si in modul `anod comun` cu acelasi tip de tranzistori NPN deoarece este folosita configuratia emitter-follower. In aceasta, tranzistorul va mentine un voltaj de 5V-0.7V intre emitor si minus. Nu este nevoie de rezistenta pe baza.


# Schematica
![Diagrama](Schematica/ValiDisplay.svg)
