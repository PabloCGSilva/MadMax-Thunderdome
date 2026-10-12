# Mad Max – Thunderdome (v0.9.0-beta1)

O Mad Max foi lançado com uma arena de luta que o jogador nunca consegue
alcançar: um domo com jaula na beira do The Big Nothing, com arquibancadas,
portão, anfitrião (o Crow Dazzle) e gerador de ondas de inimigos, desligado
antes do lançamento. Este mod liga a arena de novo e a transforma numa luta
completa.

**Esta é uma versão beta.** Faça backup dos seus saves antes de testar.

**Download:** o pacote pronto para instalar (plugin + arquivos de dados) está
na página de [Releases](https://github.com/PabloCGSilva/MadMax-Thunderdome/releases).
Este repositório tem o código-fonte do plugin.

## O que tem

* **A arena no mapa.** Uma bandeira *Pit Fight* marca o lugar desde o
  começo, e ela é um ponto de viagem rápida chamado *Thunderdome*. Também dá
  para ir dirigindo.
* **Seis ondas** de War Boys e Roadkills. Uns pulam da beira das
  arquibancadas, outros entram pelo portão da jaula: defensores com escudo,
  gimps e dois **Top Dogs** na última onda. O portão fica fechado enquanto
  você luta.
* **A Tenderloin** como adversária final. Alguns segundos depois da última
  onda ela entra na jaula. Derrube ela para vencer.
* **Torcida.** Uns 200 espectadores torcem no domo e nas arquibancadas.
* No fim da luta você é levado para fora da jaula.

## Instalação

1. Copie tudo deste pacote para a pasta do Mad Max (a que tem o
   `AVAMain.exe`), juntando as pastas `scripts` e `dropzone`.
2. O `dinput8.dll` é o Ultimate ASI Loader. Se você já tem de outro mod,
   fique com o seu.
3. Abra o jogo. O `scripts\Thunderdome.log` mostra o que o plugin encontrou.

Para desinstalar, apague o `scripts\Thunderdome.asi` e os arquivos que este
pacote colocou no `dropzone`.

## Por que tem um plugin

Quase tudo são dados do jogo no `dropzone`. O plugin pequeno faz duas coisas
que os dados não conseguem:

* Os inimigos que entram pelo portão são dos Roadkills, e o jogo só carrega
  os modelos deles perto do território deles. O plugin carrega esses
  modelos quando a arena começa. Sem ele a luta para na onda 2.
* O save guarda se cada ponto de viagem rápida está ligado. Depois de
  carregar qualquer save, o plugin liga de novo o ponto do Thunderdome.

## Requisitos e compatibilidade

* Feito e testado na versão da **GOG**. Na Steam não foi testado: o plugin
  acha as funções do jogo por padrão de bytes e espera o DRM da Steam como os
  outros mods do autor. Se não achar, ele avisa; a arena carrega mesmo assim,
  mas a luta pode parar na onda 2.
* Ele substitui, pelo dropzone: `locations/world.bin` (duas entradas da
  arena ligadas), os arquivos da arena em
  `locations/a01/activities/thunderdome/`, `global/global.blo`,
  `global/mapicons.mapiconsc`, `missions/restartpoints.restartpointsc`,
  `gui/map.guixc` e os dez `text/master_gui_*.stringlookup`. Outro mod que
  substitua algum desses arquivos não funciona junto com este. Por exemplo,
  mods que mexem no `world.bin`, como o *Respawning Scrapulances*.
* Funciona junto com Enhanced Convoys, Wasteland Storms e Enhanced
  Archangels.

## Problemas conhecidos

* **Paredes invisíveis dentro da jaula.** Alguns pontos do piso da jaula
  bloqueiam a passagem, provavelmente onde ficavam marcadores de spawn
  originais. Em investigação.
* Uma tempestade de areia chegando na arena já travou o jogo por alguns
  segundos, e depois ele voltou.
* Não testado ainda: lutar uma segunda vez na mesma sessão, e carregar um
  save feito dentro da arena durante a luta.

## O que relatar

Mande o `scripts\Thunderdome.log` (e o `Thunderdome.previous.log` se você
reiniciou o jogo), dizendo:

* a versão do jogo (GOG ou Steam),
* até onde a luta foi (número da onda, Top Dogs, Tenderloin),
* qualquer coisa que ficou parada, caiu pelo chão ou nunca apareceu.

## Compilar o plugin

Visual Studio 2022 (v143), x64 Release:

```
MSBuild Thunderdome.vcxproj /p:Configuration=Release /p:Platform=x64
```

O resultado é o `bin\Thunderdome.asi`. O `tools/gen_sigs.py` gera de novo as
assinaturas a partir de um `AVAMain.exe` da GOG (precisa de `pefile` e
`capstone`).

## Agradecimentos

* **Rick Gibbed** pelas ferramentas Gibbed do Mad Max, e **gigaHours** pelo
  fork com que elas foram feitas.
* **Tsuda Kageyu** pelo MinHook, **ThirteenAG** pelo Ultimate ASI Loader.
* **Avalanche Studios** pelo Mad Max, e por ter deixado o Thunderdome nos
  arquivos do jogo.

## Licença

MIT para o código do plugin (`thunderdome.cpp`, `tools/`), veja o `LICENSE`.
O MinHook incluído mantém a licença dele. Os arquivos de dados do pacote são
cópias modificadas dos arquivos do próprio jogo, veja o
`THIRD-PARTY-NOTICES.md`.
