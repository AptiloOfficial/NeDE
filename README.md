# NeDE — Efficient Desktop Environment

Легковесная модульная DE для Linux на базе **Wayland**.

## Компоненты

| Компонент | Описание | Стек | RAM |
|-----------|----------|------|-----|
| **NedePanel** | Панель задач | C + GTK3 + LayerShell | ~33 MB |
| **NeDE Launcher** | Лаунчер | Lua + GTK3 | ~10 MB |
| **NeDE Desktop** | Рабочий стол | C + GTK3 | ~20 MB |
| **NeDE ScreenShot** | Скриншотер | C + GTK3 | ~15 MB |
| **NeDE UAC** | Polkit-агент | C + GTK3 + Polkit | ~25 MB |

**Итого:** ~100 MB RAM для всей DE!

## Зависимости

```bash
sudo apt install \
    gtk-layer-shell \
    pipewire pipewire-pulse wireplumber \
    grim slurp wl-clipboard wlrctl \
    network-manager \
    lightdm lightdm-gtk-greeter
