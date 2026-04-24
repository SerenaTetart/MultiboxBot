import os
import sys
import time
import socket
import threading
import subprocess
import atexit
import signal

from PIL import ImageTk, Image
from tkinter import messagebox, filedialog
from pynput import keyboard
from tkinter import ttk
import tkinter as tk
import mouse

import win32gui
import win32con
import win32api
import win32job

import parser


FACTION_ALLIANCE = 0
FACTION_HORDE = 1


def rgb_hack(rgb):
    return "#%02x%02x%02x" % rgb


def getRole(player_class, spec):
    if spec in ("Protection", "Feral (bear)"):
        return 0
    elif (
        player_class == "Warrior"
        or player_class == "Rogue"
        or (player_class == "Druid" and spec == "Feral (cat)")
        or (player_class == "Paladin" and spec != "Holy")
        or (player_class == "Shaman" and spec == "Enhancement")
    ):
        return 1
    elif spec in ("Restoration", "Holy") or (player_class == "Priest" and spec != "Shadow"):
        return 3
    return 2


class Interface(tk.Tk):
    def __init__(self):
        super().__init__()

        self.resizable(False, False)
        self.title("Multibox")
        self.protocol("WM_DELETE_WINDOW", self.quit_program)
        self.attributes("-topmost", True)

        try:
            self.iconphoto(True, ImageTk.PhotoImage(Image.open(r"assets/icon.jpg")))
        except Exception:
            pass

        self.iconWoW = None
        self.iconKeybind = None
        self.iconAlliance = None
        self.iconHorde = None

        try:
            self.iconWoW = tk.PhotoImage(file="assets/iconWoW.png")
        except Exception:
            pass

        try:
            self.iconKeybind = tk.PhotoImage(file="assets/iconKeybind.png")
        except Exception:
            pass

        try:
            alliance_img = Image.open("assets/alliance.jpg").resize((16, 16))
            self.iconAlliance = ImageTk.PhotoImage(alliance_img)
        except Exception:
            pass

        try:
            horde_img = Image.open("assets/horde.jpg").resize((16, 16))
            self.iconHorde = ImageTk.PhotoImage(horde_img)
        except Exception:
            pass

        parser.init_config("config.conf")
        self.PATH_WoW = parser.get_value("config.conf", "PATH_WoW", "=")
        self.ACC_Info = parser.get_multiplevalues("config.conf", "ACC_Infos")
        self.KEYBIND_Info = parser.get_multiplevalues("config.conf", "KEYBIND_Infos")

        self.MOVEMENT_KEY = [
            win32con.VK_RIGHT,
            win32con.VK_UP,
            win32con.VK_DOWN,
            win32con.VK_LEFT,
        ]

        self.listCoord = []
        self.hwndACC = []
        self.script_running = False
        self.InMovement = [0 for _ in range(20)]
        self.indexIMG = 0
        self.NBR_ACCOUNT = 0

        self.credentialTab = None
        self.credentials_Entry = []
        self.keybindingsTab = None
        self.keybind_Entry = []
        self.listener = None

        self.Group_Label = None
        self.Players_Container = None
        self.FactionBlocks = {}
        self.PlayerFaction = [-1 for _ in range(25)]
        self.Player_Row = []
        self.Name_Label = []
        self.Class_Label = []
        self.SpecialisationList = []
        self.Specialisation_Menu = []
        self.OptionList = []

        self.serverthread = server_thread()
        self.serverthread.start()

        self._build_ui()

    # =========================
    # UI
    # =========================
    def _build_ui(self):
        tabControl = ttk.Notebook(self)
        self.tab1 = ttk.Frame(tabControl)
        self.tab2 = ttk.Frame(tabControl)

        self.tab1.grid_columnconfigure(0, weight=1)
        self.tab1.grid_columnconfigure(1, weight=1)
        self.tab1.grid_columnconfigure(2, weight=1)
        self.tab1.grid_columnconfigure(3, weight=1)
        self.tab1.grid_columnconfigure(4, weight=1)
        self.tab1.grid_columnconfigure(5, weight=1)

        self.tab2.grid_columnconfigure(0, weight=0)
        self.tab2.grid_columnconfigure(1, weight=1)
        self.tab2.grid_columnconfigure(2, weight=1)
        self.tab2.grid_columnconfigure(3, weight=1)
        self.tab2.grid_columnconfigure(4, weight=1)
        self.tab2.grid_columnconfigure(5, weight=1)
        self.tab2.grid_columnconfigure(6, weight=1)

        tabControl.pack(expand=1, fill="both")
        tabControl.add(self.tab1, text="Menu")
        tabControl.add(self.tab2, text="Options")

        self.WoWDirButton = tk.Button(
            self.tab2,
            image=self.iconWoW,
            text="...",
            width=30 if self.iconWoW is None else None,
            command=self.selectWoWDir,
        )
        self.WoWDirEntry = tk.Entry(self.tab2, state="normal")
        self.WoWDirEntry.insert(0, self.PATH_WoW)
        self.WoWDirEntry.configure(state="disabled")

        self.ModifyCredentials_Button = tk.Button(
            self.tab2,
            text="Modify credentials",
            command=self.open_credentials_tab,
            padx=5,
            pady=5,
        )
        self.ModifyKeyBindings_Button = tk.Button(
            self.tab2,
            text="Modify key bindings",
            command=self.open_keybindings_tab,
            padx=5,
            pady=5,
        )

        self.LaunchRepair_Button = tk.Button(
            self.tab1,
            text="Launch",
            width=6,
            command=self.launch_repair_clients,
            padx=5,
            pady=5,
        )

        self.ScriptOnOff_Label = tk.Label(self.tab1, text="OFF", foreground="red")
        self.NbrClient_Entry = tk.Entry(self.tab1, state="normal", justify=tk.CENTER, width=7)
        self.NbrClient_Label = tk.Label(self.tab1, text="Number clients:")

        # Mode MC unique :
        # 0 = off, 1 = no auto, 2 = auto focus/move
        self.mc_mode = tk.IntVar(value=0)

        self.MCOff_Radio = tk.Radiobutton(
            self.tab1,
            text="MC: off",
            variable=self.mc_mode,
            value=0,
            command=self.send_mc_mode,
            indicatoron=False,
            width=12,
            pady=5,
        )
        self.MCNoAuto_Radio = tk.Radiobutton(
            self.tab1,
            text="MC: no auto",
            variable=self.mc_mode,
            value=1,
            command=self.send_mc_mode,
            indicatoron=False,
            width=16,
            pady=5,
        )
        self.MCAutoMove_Radio = tk.Radiobutton(
            self.tab1,
            text="MC: auto focus/move",
            variable=self.mc_mode,
            value=2,
            command=self.send_mc_mode,
            indicatoron=False,
            width=18,
            pady=5,
        )

        self.Group_Label = tk.Label(self.tab1, text="Players detected:")
        self.Players_Container = tk.Frame(self.tab1)
        self.Players_Container.grid_columnconfigure(0, weight=1)

        self.FactionBlocks = {
            FACTION_ALLIANCE: self._create_faction_block(
                self.Players_Container,
                "Alliance",
                self.iconAlliance,
            ),
            FACTION_HORDE: self._create_faction_block(
                self.Players_Container,
                "Horde",
                self.iconHorde,
            ),
        }

        self._create_player_rows()

        self.WoWDirButton.grid(row=0, column=0, sticky="w", padx=2, pady=10)
        self.WoWDirEntry.grid(row=0, column=1, columnspan=6, sticky="ew", padx=2)

        self.ModifyCredentials_Button.grid(row=1, column=0, columnspan=3, sticky="ew", padx=2, pady=2)
        self.ModifyKeyBindings_Button.grid(row=1, column=4, columnspan=3, sticky="ew", padx=2, pady=2)

        self.ScriptOnOff_Label.grid(row=0, column=5, sticky=tk.E)
        self.LaunchRepair_Button.grid(row=1, column=0, columnspan=2, pady=2)
        self.NbrClient_Label.grid(row=1, column=3, columnspan=2, sticky=tk.E)
        self.NbrClient_Entry.grid(row=1, column=5, sticky=tk.W)

        self.MCOff_Radio.grid(row=2, column=0, columnspan=2, padx=2, pady=2)
        self.MCNoAuto_Radio.grid(row=2, column=2, columnspan=2, padx=2, pady=2)
        self.MCAutoMove_Radio.grid(row=2, column=4, columnspan=2, padx=2, pady=2)

    def _create_faction_block(self, parent, faction_name, faction_icon):
        frame = tk.Frame(parent, bd=2, relief=tk.GROOVE, padx=6, pady=4)
        frame.grid_columnconfigure(0, weight=1)

        rows = tk.Frame(frame)

        if faction_icon is not None:
            title = tk.Label(
                frame,
                image=faction_icon,
                text=f" {faction_name}",
                compound=tk.LEFT,
                font=("TkDefaultFont", 8, "bold"),
                anchor="w",
            )
        else:
            title = tk.Label(
                frame,
                text=faction_name,
                font=("TkDefaultFont", 8, "bold"),
                anchor="w",
            )

        title.grid(row=0, column=0, sticky="w", pady=(0, 3))
        rows.grid(row=1, column=0, sticky="w")

        return {
            "frame": frame,
            "rows": rows,
        }

    def _create_player_rows(self):
        self.Name_Label = []
        self.Class_Label = []
        self.SpecialisationList = []
        self.Specialisation_Menu = []
        self.OptionList = []
        self.Player_Row = []

        for i in range(25):
            row_frame = tk.Frame(self.Players_Container)

            name_label = tk.Label(row_frame, text="Null", foreground="grey", width=7, anchor="center")
            class_label = tk.Label(row_frame, text="Null", foreground="grey", width=6, anchor="center")

            spec_var = tk.StringVar(self)
            spec_var.set("Null")

            options = ["Null"]
            menu = tk.OptionMenu(row_frame, spec_var, *options)
            menu.config(width=10)

            name_label.grid(row=0, column=0, sticky="w")
            class_label.grid(row=0, column=1, sticky="w")
            menu.grid(row=0, column=2, sticky="w")

            self.Player_Row.append(row_frame)
            self.Name_Label.append(name_label)
            self.Class_Label.append(class_label)
            self.SpecialisationList.append(spec_var)
            self.Specialisation_Menu.append(menu)
            self.OptionList.append(options)

    def show_infoAccounts(self, nbr):
        self.Group_Label.grid(row=3, column=0, columnspan=6, pady=(10, 2))
        self.Players_Container.grid(row=4, column=0, columnspan=6, sticky="ew", padx=4)

        self.refresh_player_blocks()
        self.WoWDirEntry.configure(width=60)

    def refresh_player_blocks(self):
        if self.Players_Container is None or not self.FactionBlocks:
            return

        for row in self.Player_Row:
            row.grid_forget()

        visible_block_row = 0

        for faction in (FACTION_ALLIANCE, FACTION_HORDE):
            block = self.FactionBlocks[faction]

            members = [
                i
                for i in range(min(self.NBR_ACCOUNT, len(self.PlayerFaction)))
                if self.PlayerFaction[i] == faction
                and self.Name_Label[i].cget("text") not in ("", "Null")
            ]

            if not members:
                block["frame"].grid_forget()
                continue

            block["frame"].grid(
                row=visible_block_row,
                column=0,
                sticky="ew",
                padx=4,
                pady=(0, 8),
            )

            for pos, player_index in enumerate(members):
                self.Player_Row[player_index].grid(
                    in_=block["rows"],
                    row=pos // 2,
                    column=pos % 2,
                    sticky="w",
                )

            visible_block_row += 1

    # =========================
    # Réseau / protocole
    # =========================
    def sendCheckbox(self, msg):
        full_msg = "C" + msg
        self.serverthread.sendMainClient(full_msg.encode("utf-8"))

    def send_mc_mode(self):
        mode = self.mc_mode.get()
        self.sendCheckbox(f"1{1 if mode == 1 else 0}")
        self.sendCheckbox(f"2{1 if mode == 2 else 0}")

    # =========================
    # Fermeture
    # =========================
    def quit_program(self):
        try:
            self.serverthread.sendAllClients(b"QUIT")
        except Exception:
            pass

        self.script_running = False

        try:
            if self.listener is not None:
                self.listener.stop()
        except Exception:
            pass

        for hwnd in self.hwndACC:
            try:
                if win32gui.IsWindow(hwnd):
                    win32api.SendMessage(hwnd, win32con.WM_CLOSE, 0, 0)
            except Exception:
                pass

        self.serverthread.running = False

        try:
            tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tcp_socket.connect(("localhost", 50001))
            tcp_socket.close()
        except Exception:
            pass

        self.destroy()
        sys.exit(0)

    # =========================
    # Options
    # =========================
    def selectWoWDir(self):
        tmp = filedialog.askopenfile(
            title="Select your wow vanilla client",
            filetypes=[("WoW", ["exe"])],
            initialdir=self.PATH_WoW,
        )
        if tmp is not None and tmp.name != self.PATH_WoW:
            parser.modify_config("config.conf", path_wow=tmp.name)
            self.PATH_WoW = tmp.name
            self.WoWDirEntry.configure(state="normal")
            self.WoWDirEntry.delete(0, tk.END)
            self.WoWDirEntry.insert(0, tmp.name)
            self.WoWDirEntry.configure(state="disabled")

    def open_credentials_tab(self):
        if self.credentialTab and self.credentialTab.winfo_exists():
            self.credentialTab.focus_force()
            return

        self.credentialTab = tk.Toplevel(self)
        self.credentialTab.title("Credentials")
        self.credentialTab.resizable(False, False)
        self.credentialTab.attributes("-topmost", True)

        self.credentials_Entry = []

        account_Label = []
        username_Label = []
        password_Label = []

        validate_Button = tk.Button(
            self.credentialTab,
            text="Validate changes",
            command=self.modify_credentials,
        )

        y = 0
        for i in range(25):
            account_Label.append(tk.Label(self.credentialTab, text=f"Account {i + 1}"))
            username_Label.append(tk.Label(self.credentialTab, text="Username:"))
            password_Label.append(tk.Label(self.credentialTab, text="Password:"))

            user_entry = tk.Entry(self.credentialTab, width=15)
            pass_entry = tk.Entry(self.credentialTab, width=15)

            user_entry.insert(0, self.ACC_Info[i][0])
            pass_entry.insert(0, self.ACC_Info[i][1])

            self.credentials_Entry.append([user_entry, pass_entry])

            col = 0
            if i >= 20:
                col = 4
            elif i >= 10:
                col = 2

            if i in (10, 20):
                y = 0

            account_Label[i].grid(row=y, column=col + 1)
            username_Label[i].grid(row=y + 1, column=col)
            password_Label[i].grid(row=y + 2, column=col)
            user_entry.grid(row=y + 1, column=col + 1)
            pass_entry.grid(row=y + 2, column=col + 1)

            y += 3

        validate_Button.grid(row=28, column=5, padx=2)

    def modify_credentials(self):
        for i in range(25):
            self.ACC_Info[i] = (
                self.credentials_Entry[i][0].get(),
                self.credentials_Entry[i][1].get(),
            )
        parser.modify_config("config.conf", acc_info=self.ACC_Info)
        if self.credentialTab and self.credentialTab.winfo_exists():
            self.credentialTab.destroy()

    def open_keybindings_tab(self):
        if self.keybindingsTab and self.keybindingsTab.winfo_exists():
            self.keybindingsTab.focus_force()
            return

        self.keybindingsTab = tk.Toplevel(self)
        self.keybindingsTab.title("Key bindings")
        self.keybindingsTab.resizable(False, False)
        self.keybindingsTab.attributes("-topmost", True)

        self.keybind_Entry = []

        labels = [
            "Use Hearthstone:",
            "Use Mount:",
            "Toggle auto-learn:",
            "Group passive/aggressive:",
        ]

        for i, text in enumerate(labels):
            label = tk.Label(self.keybindingsTab, text=text)
            entry = tk.Entry(self.keybindingsTab, state="normal", width=15)
            entry.insert(0, self.KEYBIND_Info[i][1])
            entry.configure(state="disabled")

            btn = tk.Button(
                self.keybindingsTab,
                image=self.iconKeybind,
                text="...",
                width=30 if self.iconKeybind is None else None,
                command=lambda i=i: self.bindKey(i),
            )

            self.keybind_Entry.append(entry)

            label.grid(row=i, column=0)
            entry.grid(row=i, column=1)
            btn.grid(row=i, column=2)

    def bindKey(self, index):
        handlers = {
            0: lambda e: self._set_keybind(0, e),
            1: lambda e: self._set_keybind(1, e),
            2: lambda e: self._set_keybind(2, e),
            3: lambda e: self._set_keybind(3, e),
        }
        self.keybindingsTab.bind("<Key>", handlers[index])

    def _set_keybind(self, index, event):
        self.keybindingsTab.unbind("<Key>")
        self.keybind_Entry[index].configure(state="normal")
        self.keybind_Entry[index].delete(0, tk.END)
        self.keybind_Entry[index].insert(0, event.keysym)
        self.keybind_Entry[index].configure(state="disabled")
        self.KEYBIND_Info[index] = (str(event.keycode), event.keysym)
        parser.modify_config("config.conf", keybind_info=self.KEYBIND_Info)

    # =========================
    # WoW / clavier
    # =========================
    def send_client_txt(self, hwnd, txt):
        for c in txt:
            win32api.SendMessage(hwnd, win32con.WM_CHAR, ord(c), 0)

    def on_KeyPress(self, key):
        try:
            key_code = key.vk
        except AttributeError:
            key_code = key.value.vk

        for i in range(len(self.KEYBIND_Info)):
            if str(key_code) == self.KEYBIND_Info[i][0]:
                msg = "K" + str(i + 1)
                self.serverthread.sendGroupClients(msg.encode("utf-8"))
                break

    # =========================
    # Placement fenêtres
    # =========================
    def _row_distribution_horizontal_first(self, count):
        """
        Répartit les clients en lignes en privilégiant l'horizontal.

        Exemples :
        1  -> [1]
        2  -> [2]
        3  -> [3]
        4  -> [2, 2]
        5  -> [3, 2]
        6  -> [3, 3]
        7  -> [4, 3]
        8  -> [4, 4]
        9  -> [3, 3, 3]
        10 -> [4, 3, 3]
        25 -> [5, 5, 5, 5, 5]
        """
        if count <= 0:
            return []

        rows_count = max(1, int(count ** 0.5))

        base = count // rows_count
        extra = count % rows_count

        return [
            base + (1 if row < extra else 0)
            for row in range(rows_count)
        ]

    def _coords_for_monitor(
        self,
        monitor,
        client_indices,
        x_gap=8,
        width_increase=18,
        height_increase=23,
    ):
        """
        Génère les coordonnées d'une liste de clients sur un écran.

        La règle générale privilégie l'étirement horizontal :
        - 2 clients sur un écran => côte à côte
        - 5 clients sur un écran => 3 en haut, 2 en bas
        - 7 clients sur un écran => 4 en haut, 3 en bas
        """
        coords = {}

        if not client_indices:
            return coords

        monitor_x0, monitor_y0, monitor_x1, monitor_y1 = monitor
        monitor_width = monitor_x1 - monitor_x0
        monitor_height = monitor_y1 - monitor_y0

        rows = self._row_distribution_horizontal_first(len(client_indices))
        rows_count = len(rows)

        client_pos = 0

        for row_index, clients_in_row in enumerate(rows):
            row_y0 = monitor_y0 + (monitor_height * row_index) // rows_count
            row_y1 = monitor_y0 + (monitor_height * (row_index + 1)) // rows_count

            row_height = row_y1 - row_y0

            y = row_y0
            if row_index > 0:
                y -= height_increase - 10

            for col_index in range(clients_in_row):
                client_index = client_indices[client_pos]

                col_x0 = monitor_x0 + (monitor_width * col_index) // clients_in_row
                col_x1 = monitor_x0 + (monitor_width * (col_index + 1)) // clients_in_row

                width = (col_x1 - col_x0) + width_increase
                height = row_height + height_increase
                x = col_x0 - x_gap

                coords[client_index] = (x, y, width, height)

                client_pos += 1

        return coords

    def adapt_listCoord(self):
        self.listCoord = []

        monitors = win32api.EnumDisplayMonitors()

        idx_primary = next(
            i
            for i, m in enumerate(monitors)
            if win32api.GetMonitorInfo(m[0])["Flags"] & win32con.MONITORINFOF_PRIMARY
        )

        x_gap = 8
        width_increase = 18
        height_increase = 23

        coords_by_index = {}

        if len(monitors) == 1:
            monitor = monitors[0][2]
            client_indices = list(range(self.NBR_ACCOUNT))

            coords_by_index.update(
                self._coords_for_monitor(
                    monitor,
                    client_indices,
                    x_gap,
                    width_increase,
                    height_increase,
                )
            )

        else:
            main_monitor = idx_primary
            other_monitors = [i for i in range(len(monitors)) if i != main_monitor]

            if not other_monitors:
                return

            # Même comportement que ton code d'origine :
            # écran secondaire = écran le plus à droite.
            second_monitor = max(other_monitors, key=lambda i: monitors[i][2][0])

            m1 = monitors[main_monitor][2]
            m2 = monitors[second_monitor][2]

            # Clients principaux :
            # 0, 5, 10, 15, 20...
            #
            # Exemple avec 7 clients :
            # écran principal => clients 0 et 5.
            main_clients = [
                i
                for i in range(self.NBR_ACCOUNT)
                if i % 5 == 0
            ]

            # Tous les autres vont sur l'écran secondaire.
            #
            # Exemple avec 7 clients :
            # écran secondaire => clients 1, 2, 3, 4, 6.
            second_clients = [
                i
                for i in range(self.NBR_ACCOUNT)
                if i % 5 != 0
            ]

            coords_by_index.update(
                self._coords_for_monitor(
                    m1,
                    main_clients,
                    x_gap,
                    width_increase,
                    height_increase,
                )
            )

            coords_by_index.update(
                self._coords_for_monitor(
                    m2,
                    second_clients,
                    x_gap,
                    width_increase,
                    height_increase,
                )
            )

        # Important :
        # listCoord[index] doit correspondre au client index.
        for i in range(self.NBR_ACCOUNT):
            if i in coords_by_index:
                self.listCoord.append(coords_by_index[i])

    # =========================
    # Launch / repair clients
    # =========================
    def launch_repair_clients(self):
        nbr_monitor = win32api.GetSystemMetrics(win32con.SM_CMONITORS)

        if self.LaunchRepair_Button.cget("text") == "Launch":
            value = self.NbrClient_Entry.get().strip()

            if value == "":
                messagebox.showerror("Error", "You must specify the number of clients !")
                return

            try:
                nbr = int(value)
            except ValueError:
                messagebox.showerror("Error", "Number of clients must be an integer !")
                return

            if nbr <= 0:
                messagebox.showerror("Error", f"You can't have {nbr} clients !")
                return

            self.NBR_ACCOUNT = nbr
            self.NbrClient_Entry.configure(state="disabled")
            self.LaunchRepair_Button.config(text="Repair")
            self.adapt_listCoord()

            for i in range(self.NBR_ACCOUNT):
                hwnd = self._spawn_or_find_client(i)
                self.hwndACC.append(hwnd)
                self._place_client_window(i, hwnd, nbr_monitor)

            for i in range(self.NBR_ACCOUNT):
                self._login_client(i)

            self.show_infoAccounts(self.NBR_ACCOUNT)

        else:
            for i in range(self.NBR_ACCOUNT):
                hwnd = self.hwndACC[i] if i < len(self.hwndACC) else 0

                if not hwnd or not win32gui.IsWindow(hwnd):
                    hwnd = self._spawn_or_find_client(i)
                    if i < len(self.hwndACC):
                        self.hwndACC[i] = hwnd
                    else:
                        self.hwndACC.append(hwnd)
                    self._login_client(i)

                self._place_client_window(i, hwnd, nbr_monitor)

    def _spawn_or_find_client(self, index):
        p1 = subprocess.Popen(["./src/Bootstrap.exe", self.PATH_WoW])
        p1.wait()

        hwnd = win32gui.FindWindow(None, "World of Warcraft")
        while hwnd == 0:
            hwnd = win32gui.FindWindow(None, "World of Warcraft")
            time.sleep(0.1)

        time.sleep(0.5)
        win32gui.SetWindowText(hwnd, f"WoW{index + 1}")
        return hwnd

    def _login_client(self, index):
        hwnd = self.hwndACC[index]
        self.send_client_txt(hwnd, self.ACC_Info[index][0])

        win32api.SendMessage(hwnd, win32con.WM_KEYDOWN, win32con.VK_TAB, 0)
        win32api.SendMessage(hwnd, win32con.WM_KEYUP, win32con.VK_TAB, 0)

        self.send_client_txt(hwnd, self.ACC_Info[index][1])
        time.sleep(0.1)

        win32api.SendMessage(hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
        win32api.SendMessage(hwnd, win32con.WM_KEYUP, win32con.VK_RETURN, 0)

    def _place_client_window(self, index, hwnd, nbr_monitor):
        # Plein écran conservé :
        # - 1 client
        # - ou <= 5 clients avec au moins 2 écrans :
        #   le client principal reste plein écran.
        if index == 0 and ((self.NBR_ACCOUNT <= 5 and nbr_monitor >= 2) or (self.NBR_ACCOUNT == 1)):
            win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
            return

        if index < len(self.listCoord):
            x, y, w, h = self.listCoord[index]
            win32gui.MoveWindow(hwnd, x, y, w, h, True)

        # Plein écran conservé :
        # cas spécial d'origine avec 2 clients et 2 écrans.
        if index == 1 and self.NBR_ACCOUNT == 2 and nbr_monitor == 2:
            win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)

    # =========================
    # Bot
    # =========================
    def activateBot(self):
        self.MOVEMENT_KEY = [
            win32con.VK_RIGHT,
            win32con.VK_UP,
            win32con.VK_DOWN,
            win32con.VK_LEFT,
        ]

        if self.script_running:
            self.script_running = False
            self.serverthread.sendAllClients(b"Bot: OFF")
            self.ScriptOnOff_Label.config(text="OFF", foreground="red")
        else:
            self.script_running = True
            self.serverthread.sendAllClients(b"Bot: ON")
            self.ScriptOnOff_Label.config(text="ON", foreground="green")

    # =========================
    # UI update helpers
    # =========================
    def update_player_class(self, index, name, player_class, options, color, faction):
        if index >= len(self.PlayerFaction):
            return

        self.PlayerFaction[index] = faction if faction in (FACTION_ALLIANCE, FACTION_HORDE) else -1

        self.Name_Label[index].config(text=name, foreground="black")
        self.Class_Label[index].config(text=player_class, foreground=color)

        self.OptionList[index] = options

        menu = self.Specialisation_Menu[index]["menu"]
        menu.delete(0, tk.END)

        for option in options:
            menu.add_command(
                label=option,
                command=tk._setit(self.SpecialisationList[index], option),
            )

        self.SpecialisationList[index].set(options[0] if options else "Null")
        self.refresh_player_blocks()

    def clear_player(self, index):
        if index >= len(self.PlayerFaction):
            return

        self.PlayerFaction[index] = -1

        self.Name_Label[index].config(text="Null", foreground="grey")
        self.Class_Label[index].config(text="Null", foreground="grey")

        self.OptionList[index] = ["Null"]

        menu = self.Specialisation_Menu[index]["menu"]
        menu.delete(0, tk.END)
        menu.add_command(
            label="Null",
            command=tk._setit(self.SpecialisationList[index], "Null"),
        )

        self.SpecialisationList[index].set("Null")
        self.Player_Row[index].grid_forget()
        self.refresh_player_blocks()


class client_thread(threading.Thread):
    CLASS_OPTIONS = {
        "Druid": (["Balance", "Feral (bear)", "Feral (cat)", "Restoration"], "orange"),
        "Hunter": (["Beast Mastery", "Marksmanship", "Survival"], "green"),
        "Mage": (["Arcane", "Fire", "Frost"], rgb_hack((0, 210, 255))),
        "Paladin": (["Holy", "Protection", "Retribution"], rgb_hack((255, 0, 122))),
        "Priest": (["Discipline", "Holy", "Shadow"], rgb_hack((210, 210, 210))),
        "Rogue": (["Assassination", "Combat", "Subtlety"], rgb_hack((255, 210, 0))),
        "Shaman": (["Elemental", "Enhancement", "Restoration"], "blue"),
        "Warlock": (["Affliction", "Demonology", "Destruction"], "purple"),
        "Warrior": (["Arms", "Fury", "Protection"], "brown"),
        "Null": (["Null"], "grey"),
    }

    def __init__(self, index, conn, addr):
        super().__init__(daemon=True)
        self.running = True
        self.conn = conn
        self.addr = addr
        self.index = index
        self.currentSpec = "Null"
        self.Name = ""
        self.Class = ""
        self.Faction = -1
        self.CraftSkill = [0, 0]

    def run(self):
        interface.after(500, self.checkSpecChange)

        while self.running:
            try:
                data = self.conn.recv(128)
                if not data:
                    break

                msg = data.decode("utf-8", errors="ignore")

                if "Class" in msg:
                    self._handle_class_message(msg)
                elif "Craft" in msg:
                    self._handle_craft_message(msg)

            except Exception as e:
                print(f"{self.addr[0]}:{self.addr[1]} - {e}")
                break

        self.running = False
        interface.serverthread.clients[self.index] = None

        try:
            self.conn.close()
        except Exception:
            pass

        try:
            if self.index < len(interface.hwndACC) and win32gui.IsWindow(interface.hwndACC[self.index]):
                win32api.SendMessage(interface.hwndACC[self.index], win32con.WM_CLOSE, 0, 0)
        except Exception:
            pass

        interface.after(0, lambda: interface.clear_player(self.index))

        print(f"[-] Client disconnected: {self.addr[0]}:{self.addr[1]}")

    def _handle_class_message(self, msg):
        ind = msg.find("Class")
        ind2 = msg.find("Faction")

        if ind == -1 or ind2 == -1 or ind2 <= ind:
            return

        self.Name = msg[5:ind - 1]
        self.Class = msg[ind + 6:ind2 - 1]

        try:
            self.Faction = int(msg[ind2 + 8:].strip())
        except ValueError:
            self.Faction = -1

        options, color = self.CLASS_OPTIONS.get(self.Class, (["Null"], "grey"))
        self.currentSpec = "Null"

        interface.after(
            0,
            lambda: interface.update_player_class(
                self.index,
                self.Name,
                self.Class,
                options,
                color,
                self.Faction,
            ),
        )

    def _handle_craft_message(self, msg):
        try:
            self.CraftSkill[0] = int(msg[5])
            self.CraftSkill[1] = int(msg[6])
        except Exception:
            return

        # Format envoyé au serveur C++ :
        # Craft{type}{id:02}{len:02}{skill}{name}
        #
        # type = 0 ou 1
        # id   = index du joueur sur 2 chiffres
        # len  = longueur complète du nom sur 2 chiffres
        #
        # Important :
        # on n'utilise plus len(self.Name) - 1, sinon le nom est tronqué.
        out0 = f"Craft0{str(self.index).zfill(2)}{str(len(self.Name)).zfill(2)}{self.CraftSkill[0]}{self.Name}"
        out1 = f"Craft1{str(self.index).zfill(2)}{str(len(self.Name)).zfill(2)}{self.CraftSkill[1]}{self.Name}"

        interface.serverthread.sendAllClients(out0.encode("utf-8"))
        interface.serverthread.sendAllClients(out1.encode("utf-8"))

    def checkSpecChange(self):
        if not self.running:
            return

        spec_tmp = interface.SpecialisationList[self.index].get()

        if self.currentSpec != spec_tmp and len(self.Name) > 0:
            self.currentSpec = spec_tmp

            for i in range(interface.NBR_ACCOUNT):
                ct = interface.serverthread.clients_thread[i] if i < len(interface.serverthread.clients_thread) else None
                if ct is None or len(ct.Name) <= 0:
                    continue

                role_nbr = getRole(ct.Class, ct.currentSpec)

                # Format envoyé au serveur C++ :
                # Role{role}_{id:02}_{len:02}_{name}
                #
                # Important :
                # on n'utilise plus len(ct.Name) - 1, sinon le nom est tronqué.
                msg = f"Role{role_nbr}_{str(i).zfill(2)}_{str(len(ct.Name)).zfill(2)}_{ct.Name}"

                interface.serverthread.sendAllClients(msg.encode("utf-8"))

            for i, option in enumerate(interface.OptionList[self.index]):
                if self.currentSpec == option:
                    msg = f"Spec: {i} "
                    try:
                        self.conn.send(msg.encode("utf-8"))
                    except Exception:
                        self.running = False
                    time.sleep(0.01)
                    break

        interface.after(500, self.checkSpecChange)


class ChildServer:
    def __init__(self, exe_path):
        self.proc = subprocess.Popen([exe_path])
        self.job = win32job.CreateJobObject(None, "")

        info = win32job.QueryInformationJobObject(
            self.job,
            win32job.JobObjectExtendedLimitInformation,
        )
        info["BasicLimitInformation"]["LimitFlags"] |= win32job.JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
        win32job.SetInformationJobObject(
            self.job,
            win32job.JobObjectExtendedLimitInformation,
            info,
        )

        hProcess = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, False, self.proc.pid)
        win32job.AssignProcessToJobObject(self.job, hProcess)

        atexit.register(self._cleanup)
        signal.signal(signal.SIGINT, lambda *_: self._cleanup(exit_code=130))
        signal.signal(signal.SIGTERM, lambda *_: self._cleanup(exit_code=143))

    def _cleanup(self, exit_code=None):
        try:
            if self.proc and self.proc.poll() is None:
                try:
                    win32api.CloseHandle(self.job)
                except Exception:
                    pass

                try:
                    if self.proc.poll() is None:
                        self.proc.terminate()
                except Exception:
                    pass
        finally:
            if exit_code is not None:
                raise SystemExit(exit_code)


class server_thread(threading.Thread):
    def __init__(self):
        super().__init__(daemon=True)
        self.running = True
        self.clients = []
        self.clients_thread = []
        self.tcp_socket = None

    def run(self):
        self.tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.tcp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.tcp_socket.bind(("localhost", 50001))
        self.tcp_socket.listen(25)

        while self.running:
            try:
                conn, addr = self.tcp_socket.accept()
            except Exception:
                break

            if not self.running:
                try:
                    conn.close()
                except Exception:
                    pass
                break

            free_index = next((i for i, c in enumerate(self.clients) if c is None), None)

            if free_index is None:
                self.clients.append((conn, addr))
                ct = client_thread(len(self.clients) - 1, conn, addr)
                self.clients_thread.append(ct)
                ct.start()
            else:
                self.clients[free_index] = (conn, addr)
                ct = client_thread(free_index, conn, addr)
                self.clients_thread[free_index] = ct
                ct.start()

            print(f"[+] New client: {addr[0]}:{addr[1]}")

        for ct in self.clients_thread:
            if ct is not None:
                ct.running = False
                try:
                    ct.conn.close()
                except Exception:
                    pass

        try:
            if self.tcp_socket:
                self.tcp_socket.close()
        except Exception:
            pass

        print("server over...")
        interface.after(0, interface.destroy)

    def _safe_send(self, client_tuple, msg):
        if client_tuple is None:
            return False

        try:
            # TCP ne conserve pas les frontières entre messages.
            # On ajoute donc un séparateur clair pour les clients C++.
            if not msg.endswith(b"\n"):
                msg += b"\n"

            client_tuple[0].sendall(msg)
            time.sleep(0.01)
            return True
        except Exception:
            return False

    def sendAllClients(self, msg):
        for i, client in enumerate(self.clients):
            if client is not None and not self._safe_send(client, msg):
                self.clients[i] = None

    def sendGroupClients(self, msg, index=-1):
        if not self.clients:
            return

        if index == -1:
            foreground = win32gui.GetForegroundWindow()
            for i in range(((interface.NBR_ACCOUNT - 1) // 5) + 1):
                start = i * 5
                end = min(start + 5, interface.NBR_ACCOUNT)
                if foreground in interface.hwndACC[start:end]:
                    for y in range(start, end):
                        if y < len(self.clients) and self.clients[y] is not None:
                            if not self._safe_send(self.clients[y], msg):
                                self.clients[y] = None
                    return
        else:
            start = index - (index % 5)
            end = min(start + 5, interface.NBR_ACCOUNT)
            for y in range(start, end):
                if y < len(self.clients) and self.clients[y] is not None:
                    if not self._safe_send(self.clients[y], msg):
                        self.clients[y] = None

    def sendMainClient(self, msg):
        if not self.clients:
            return

        if interface.NBR_ACCOUNT == 1:
            if len(self.clients) > 0 and self.clients[0] is not None:
                if not self._safe_send(self.clients[0], msg):
                    self.clients[0] = None
            return

        for i in range(((interface.NBR_ACCOUNT - 1) // 5) + 1):
            idx = i * 5
            if idx < len(self.clients) and self.clients[idx] is not None:
                if not self._safe_send(self.clients[idx], msg):
                    self.clients[idx] = None


if __name__ == "__main__":
    child = ChildServer(".\\src\\ServerNavigation.exe")
    interface = Interface()

    mouse.on_middle_click(interface.activateBot)

    listener = keyboard.Listener(on_press=interface.on_KeyPress)
    interface.listener = listener
    listener.start()

    interface.mainloop()

    print("\nYou can close this window...")
    sys.exit(0)