 <img width="1366" height="768" alt="screenshot_2026-04-14_01-07-34" src="https://github.com/user-attachments/assets/3654b3d6-0c99-44d4-96af-435675c163e8" />
# NeDE — Nede Efficient Desktop Environment

Lightweight modular DE for Linux based on **Wayland**.

## Components

| Component | Description | Stack | RAM |
| --- | --- | --- | --- |
| **NedePanel** | Taskbar | C + GTK3 + LayerShell | ~33 MB |
| **NeDE Launcher** | App Launcher | Lua + GTK3 | ~10 MB |
| **NeDE Desktop** | Desktop | C + GTK3 | ~20 MB |
| **NeDE ScreenShot** | Screenshot tool | C + GTK3 | ~15 MB |
| **NeDE UAC** | Polkit agent | C + GTK3 + Polkit | ~25 MB |
| **LabWC** | Compositor Wayland | wlroots | ~80 MB |
**Total:** ~200 MB RAM for the entire DE. **

## Dependencies

```bash
sudo apt install \
    gtk-layer-shell \
    pipewire pipewire-pulse wireplumber \
    grim slurp wl-clipboard wlrctl \
    network-manager \
    lightdm lightdm-gtk-greeter
```

Русский
# NeDE — Nede Efficient Desktop Environment

Легковесная модульная DE для Linux на базе **Wayland**.

## Компоненты

| Компонент | Описание | Стек | RAM |
|-----------|----------|------|-----|
| **NedePanel** | Панель задач | C + GTK3 + LayerShell | ~33 MB |
| **NeDE Launcher** | Лаунчер | Lua + GTK3 | ~10 MB |
| **NeDE Desktop** | Рабочий стол | C + GTK3 | ~20 MB |
| **NeDE ScreenShot** | Скриншотер | C + GTK3 | ~15 MB |
| **NeDE UAC** | Polkit-агент | C + GTK3 + Polkit | ~25 MB |
| **LabWC** | Композитор Wayland | wlroots | ~80 MB |

**Итого:** ~200 MB RAM для всей DE! *

## Зависимости

```bash
sudo apt install \
    gtk-layer-shell \
    pipewire pipewire-pulse wireplumber \
    grim slurp wl-clipboard wlrctl \
    network-manager \
    lightdm lightdm-gtk-greeter

```
!!!
 * Потребление ОЗУ NeDE может быть разной или неточной. Это зависит от вашей ОЗУ, например на 16 ГБ у вас потребление может быть 700 мб - это нормально для Linux.
 ** RAM usage of NeDE may vary or be inaccurate. It depends on your total RAM—for example, with 16 GB installed, memory usage might reach around 700 MB, which is normal for Linux.
