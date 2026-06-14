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

MAX_ACCOUNTS = 40
GROUP_SIZE = 5

SERVER_HOST = "localhost"
SERVER_PORT = 50001


def rgb_hack(rgb):
    return "#%02x%02x%02x" % rgb


def getRole(player_class, spec):
    if spec in ("Protection", "Feral (bear)"):
        return 0

    if (
        player_class == "Warrior"
        or player_class == "Rogue"
        or (player_class == "Druid" and spec == "Feral (cat)")
        or (player_class == "Paladin" and spec != "Holy")
        or (player_class == "Shaman" and spec == "Enhancement")
    ):
        return 1

    if spec in ("Restoration", "Holy") or (player_class == "Priest" and spec != "Shadow"):
        return 3

    return 2


class Interface(tk.Tk):
    def __init__(self):
        super().__init__()

        self.resizable(False, False)
        self.title("Multibox")
        self.protocol("WM_DELETE_WINDOW", self.quit_program)
        self.attributes("-topmost", True)

        self.iconWoW = self.load_photo("assets/iconWoW.png")
        self.iconKeybind = self.load_photo("assets/iconKeybind.png")
        self.iconAlliance = self.load_resized_photo("assets/alliance.jpg", 16, 16)
        self.iconHorde = self.load_resized_photo("assets/horde.jpg", 16, 16)

        try:
            self.iconphoto(True, ImageTk.PhotoImage(Image.open(r"assets/icon.jpg")))
        except Exception:
            pass

        parser.init_config("config.conf")

        self.PATH_WoW = parser.get_value("config.conf", "PATH_WoW", "=")
        self.ACC_Info = parser.get_multiplevalues("config.conf", "ACC_Infos")
        self.KEYBIND_Info = parser.get_multiplevalues("config.conf", "KEYBIND_Infos")

        while len(self.ACC_Info) < MAX_ACCOUNTS:
            self.ACC_Info.append(("", ""))

        while len(self.KEYBIND_Info) < 4:
            self.KEYBIND_Info.append(("", ""))

        self.MOVEMENT_KEY = [
            win32con.VK_RIGHT,
            win32con.VK_UP,
            win32con.VK_DOWN,
            win32con.VK_LEFT,
        ]

        self.listCoord = []
        self.hwndACC = []
        self.script_running = False
        self.InMovement = [0 for _ in range(MAX_ACCOUNTS)]
        self.indexIMG = 0
        self.NBR_ACCOUNT = 0

        self.listener = None
        self.credentialTab = None
        self.keybindingsTab = None
        self.credentials_Entry = []
        self.keybind_Entry = []

        self.Group_Label = None
        self.Players_Container = None
        self.FactionBlocks = {}
        self.PlayerFaction = [-1 for _ in range(MAX_ACCOUNTS)]
        self.Player_Row = []
        self.Name_Label = []
        self.Class_Label = []
        self.SpecialisationList = []
        self.Specialisation_Menu = []
        self.OptionList = []

        self.mc_mode = tk.IntVar(value=0)
        self.spec_check_after_id = None
        self.role_cache = {}
        self.refresh_players_pending = False

        self.serverthread = server_thread()
        self.serverthread.start()

        self._build_ui()
        self.spec_check_after_id = self.after(500, self.check_all_specs)

    # =========================
    # Assets
    # =========================
    def load_photo(self, path):
        try:
            return tk.PhotoImage(file=path)
        except Exception:
            return None

    def load_resized_photo(self, path, width, height):
        try:
            return ImageTk.PhotoImage(Image.open(path).resize((width, height)))
        except Exception:
            return None

    # =========================
    # UI
    # =========================
    def _build_ui(self):
        tabControl = ttk.Notebook(self)
        self.tab1 = ttk.Frame(tabControl)
        self.tab2 = ttk.Frame(tabControl)

        for col in range(6):
            self.tab1.grid_columnconfigure(col, weight=1)

        for col in range(9):
            self.tab2.grid_columnconfigure(col, weight=1 if col else 0)

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

        self.MC_Radios = []

        for text, value, width in (
            ("MC: default", 0, 12),
            ("MC: off", 1, 16),
            ("MC: auto focus/move", 2, 18),
        ):
            radio = tk.Radiobutton(
                self.tab1,
                text=text,
                variable=self.mc_mode,
                value=value,
                command=self.send_mc_mode,
                indicatoron=False,
                width=width,
                pady=5,
            )
            self.MC_Radios.append(radio)

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
        self.WoWDirEntry.grid(row=0, column=1, columnspan=8, sticky="ew", padx=2)

        self.ModifyCredentials_Button.grid(row=1, column=0, columnspan=4, sticky="ew", padx=2, pady=2)
        self.ModifyKeyBindings_Button.grid(row=1, column=5, columnspan=4, sticky="ew", padx=2, pady=2)

        self.ScriptOnOff_Label.grid(row=0, column=5, sticky=tk.E)
        self.LaunchRepair_Button.grid(row=1, column=0, columnspan=2, pady=2)
        self.NbrClient_Label.grid(row=1, column=3, columnspan=2, sticky=tk.E)
        self.NbrClient_Entry.grid(row=1, column=5, sticky=tk.W)

        for i, radio in enumerate(self.MC_Radios):
            radio.grid(row=2, column=i * 2, columnspan=2, padx=2, pady=2)

    def _create_faction_block(self, parent, faction_name, faction_icon):
        frame = tk.Frame(parent, bd=2, relief=tk.GROOVE, padx=6, pady=4)
        frame.grid_columnconfigure(0, weight=1)

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

        rows = tk.Frame(frame)

        title.grid(row=0, column=0, sticky="w", pady=(0, 3))
        rows.grid(row=1, column=0, sticky="w")

        return {"frame": frame, "rows": rows}

    def _create_player_rows(self):
        for _ in range(MAX_ACCOUNTS):
            row_frame = tk.Frame(self.Players_Container)

            name_label = tk.Label(row_frame, text="Null", foreground="grey", width=7, anchor="center")
            class_label = tk.Label(row_frame, text="Null", foreground="grey", width=6, anchor="center")

            spec_var = tk.StringVar(self)
            spec_var.set("Null")

            menu = tk.OptionMenu(row_frame, spec_var, "Null")
            menu.config(width=10)

            name_label.grid(row=0, column=0, sticky="w")
            class_label.grid(row=0, column=1, sticky="w")
            menu.grid(row=0, column=2, sticky="w")

            self.Player_Row.append(row_frame)
            self.Name_Label.append(name_label)
            self.Class_Label.append(class_label)
            self.SpecialisationList.append(spec_var)
            self.Specialisation_Menu.append(menu)
            self.OptionList.append(["Null"])

    def show_infoAccounts(self, nbr):
        self.Group_Label.grid(row=3, column=0, columnspan=6, pady=(10, 2))
        self.Players_Container.grid(row=4, column=0, columnspan=6, sticky="ew", padx=4)
        self.WoWDirEntry.configure(width=60)
        self.schedule_player_refresh()

    def schedule_player_refresh(self):
        if not self.refresh_players_pending:
            self.refresh_players_pending = True
            self.after_idle(self.refresh_player_blocks)

    def refresh_player_blocks(self):
        self.refresh_players_pending = False

        for row in self.Player_Row:
            row.grid_forget()

        visible_block_row = 0

        for faction in (FACTION_ALLIANCE, FACTION_HORDE):
            block = self.FactionBlocks[faction]

            members = [
                i
                for i in range(min(self.NBR_ACCOUNT, MAX_ACCOUNTS))
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
        self.serverthread.sendMainClients(("C" + msg).encode("utf-8"))

    def send_mc_mode(self):
        mode = self.mc_mode.get()

        self.sendCheckbox(f"1{1 if mode == 1 else 0}")
        self.sendCheckbox(f"2{1 if mode == 2 else 0}")

    # =========================
    # Fermeture
    # =========================
    def quit_program(self):
        try:
            if self.spec_check_after_id is not None:
                self.after_cancel(self.spec_check_after_id)
                self.spec_check_after_id = None
        except Exception:
            pass

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
            tcp_socket.connect((SERVER_HOST, SERVER_PORT))
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

        if tmp is None or tmp.name == self.PATH_WoW:
            return

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

        for i in range(MAX_ACCOUNTS):
            column_index = i // 10
            row_index = i % 10

            col = column_index * 2
            y = row_index * 3

            account_Label = tk.Label(self.credentialTab, text=f"Account {i + 1}")
            username_Label = tk.Label(self.credentialTab, text="Username:")
            password_Label = tk.Label(self.credentialTab, text="Password:")

            user_entry = tk.Entry(self.credentialTab, width=15)
            pass_entry = tk.Entry(self.credentialTab, width=15)

            user_entry.insert(0, self.ACC_Info[i][0])
            pass_entry.insert(0, self.ACC_Info[i][1])

            self.credentials_Entry.append([user_entry, pass_entry])

            account_Label.grid(row=y, column=col + 1)
            username_Label.grid(row=y + 1, column=col)
            password_Label.grid(row=y + 2, column=col)
            user_entry.grid(row=y + 1, column=col + 1)
            pass_entry.grid(row=y + 2, column=col + 1)

        validate_Button = tk.Button(
            self.credentialTab,
            text="Validate changes",
            command=self.modify_credentials,
        )

        validate_Button.grid(row=31, column=7, padx=2, pady=4)

    def modify_credentials(self):
        for i in range(MAX_ACCOUNTS):
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
                command=lambda index=i: self.bindKey(index),
            )

            self.keybind_Entry.append(entry)

            label.grid(row=i, column=0)
            entry.grid(row=i, column=1)
            btn.grid(row=i, column=2)

    def bindKey(self, index):
        self.keybindingsTab.bind("<Key>", lambda event, i=index: self._set_keybind(i, event))

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

        for i, keybind in enumerate(self.KEYBIND_Info):
            if str(key_code) == keybind[0]:
                self.serverthread.sendGroupClients(f"K{i + 1}".encode("utf-8"))
                break

    # =========================
    # Placement fenêtres
    # =========================
    def _row_distribution_horizontal_first(self, count):
        if count <= 0:
            return []

        rows_count = max(1, int(count ** 0.5))
        base = count // rows_count
        extra = count % rows_count

        return [base + (1 if row < extra else 0) for row in range(rows_count)]

    def _coords_for_monitor(self, monitor, client_indices, x_gap=8, width_increase=18, height_increase=23):
        if not client_indices:
            return {}

        monitor_x0, monitor_y0, monitor_x1, monitor_y1 = monitor
        monitor_width = monitor_x1 - monitor_x0
        monitor_height = monitor_y1 - monitor_y0

        coords = {}
        rows = self._row_distribution_horizontal_first(len(client_indices))
        client_pos = 0

        for row_index, clients_in_row in enumerate(rows):
            row_y0 = monitor_y0 + (monitor_height * row_index) // len(rows)
            row_y1 = monitor_y0 + (monitor_height * (row_index + 1)) // len(rows)

            y = row_y0 - (height_increase - 10 if row_index > 0 else 0)
            height = (row_y1 - row_y0) + height_increase

            for col_index in range(clients_in_row):
                client_index = client_indices[client_pos]

                col_x0 = monitor_x0 + (monitor_width * col_index) // clients_in_row
                col_x1 = monitor_x0 + (monitor_width * (col_index + 1)) // clients_in_row

                coords[client_index] = (
                    col_x0 - x_gap,
                    y,
                    (col_x1 - col_x0) + width_increase,
                    height,
                )

                client_pos += 1

        return coords

    def adapt_listCoord(self):
        self.listCoord = []

        monitors = win32api.EnumDisplayMonitors()

        if not monitors:
            return

        idx_primary = next(
            i
            for i, m in enumerate(monitors)
            if win32api.GetMonitorInfo(m[0])["Flags"] & win32con.MONITORINFOF_PRIMARY
        )

        coords_by_index = {}
        monitor_count = len(monitors)

        if monitor_count == 1:
            coords_by_index.update(
                self._coords_for_monitor(
                    monitors[0][2],
                    list(range(self.NBR_ACCOUNT)),
                )
            )

        elif monitor_count == 2:
            second_monitor = max(
                [i for i in range(monitor_count) if i != idx_primary],
                key=lambda i: monitors[i][2][0],
            )

            main_clients = [i for i in range(self.NBR_ACCOUNT) if i % GROUP_SIZE == 0]
            second_clients = [i for i in range(self.NBR_ACCOUNT) if i % GROUP_SIZE != 0]

            coords_by_index.update(self._coords_for_monitor(monitors[idx_primary][2], main_clients))
            coords_by_index.update(self._coords_for_monitor(monitors[second_monitor][2], second_clients))

        else:
            primary_x0 = monitors[idx_primary][2][0]
            others = [i for i in range(monitor_count) if i != idx_primary]

            left_candidates = [i for i in others if monitors[i][2][0] < primary_x0]
            right_candidates = [i for i in others if monitors[i][2][0] >= primary_x0]

            left_monitor = (
                max(left_candidates, key=lambda i: monitors[i][2][0])
                if left_candidates
                else min(others, key=lambda i: monitors[i][2][0])
            )

            right_monitor = (
                min(right_candidates, key=lambda i: monitors[i][2][0])
                if right_candidates
                else max(others, key=lambda i: monitors[i][2][0])
            )

            if left_monitor == right_monitor and len(others) >= 2:
                sorted_others = sorted(others, key=lambda i: monitors[i][2][0])
                left_monitor = sorted_others[0]
                right_monitor = sorted_others[-1]

            main_clients = [i for i in range(self.NBR_ACCOUNT) if i % GROUP_SIZE == 0]

            groups = []
            for start in range(0, self.NBR_ACCOUNT, GROUP_SIZE):
                followers = list(range(start + 1, min(start + GROUP_SIZE, self.NBR_ACCOUNT)))
                if followers:
                    groups.append(followers)

            right_group_count = (len(groups) + 1) // 2

            right_clients = [i for group in groups[:right_group_count] for i in group]
            left_clients = [i for group in groups[right_group_count:] for i in group]

            coords_by_index.update(self._coords_for_monitor(monitors[left_monitor][2], left_clients))
            coords_by_index.update(self._coords_for_monitor(monitors[idx_primary][2], main_clients))
            coords_by_index.update(self._coords_for_monitor(monitors[right_monitor][2], right_clients))

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

            if nbr > MAX_ACCOUNTS:
                messagebox.showerror("Error", f"Maximum number of clients is {MAX_ACCOUNTS} !")
                return

            self.NBR_ACCOUNT = nbr
            self.NbrClient_Entry.configure(state="disabled")
            self.LaunchRepair_Button.config(text="Repair")

            self.adapt_listCoord()

            for i in range(self.NBR_ACCOUNT):
                self.serverthread.reserve_index(i)

                hwnd = self._spawn_or_find_client(i)

                if i < len(self.hwndACC):
                    self.hwndACC[i] = hwnd
                else:
                    self.hwndACC.append(hwnd)

                self._place_client_window(i, hwnd, nbr_monitor)

            for i in range(self.NBR_ACCOUNT):
                self._login_client(i)

            self.show_infoAccounts(self.NBR_ACCOUNT)
            return

        self.adapt_listCoord()

        for i in range(self.NBR_ACCOUNT):
            hwnd = self.hwndACC[i] if i < len(self.hwndACC) else 0

            if not hwnd or not win32gui.IsWindow(hwnd):
                self.serverthread.reserve_index(i)

                hwnd = self._spawn_or_find_client(i)

                if i < len(self.hwndACC):
                    self.hwndACC[i] = hwnd
                else:
                    self.hwndACC.append(hwnd)

                self._place_client_window(i, hwnd, nbr_monitor)
                self._login_client(i)
                continue

            self._place_client_window(i, hwnd, nbr_monitor)

    def _spawn_or_find_client(self, index):
        p1 = subprocess.Popen(["./src/Bootstrap.exe", self.PATH_WoW])
        p1.wait()

        hwnd = win32gui.FindWindow(None, "World of Warcraft")

        while hwnd == 0:
            time.sleep(0.1)
            hwnd = win32gui.FindWindow(None, "World of Warcraft")

        time.sleep(0.5)
        win32gui.SetWindowText(hwnd, f"WoW{index + 1}")

        return hwnd

    def _login_client(self, index):
        if index >= len(self.hwndACC):
            return

        hwnd = self.hwndACC[index]

        if not hwnd or not win32gui.IsWindow(hwnd):
            return

        self.send_client_txt(hwnd, self.ACC_Info[index][0])

        win32api.SendMessage(hwnd, win32con.WM_KEYDOWN, win32con.VK_TAB, 0)
        win32api.SendMessage(hwnd, win32con.WM_KEYUP, win32con.VK_TAB, 0)

        self.send_client_txt(hwnd, self.ACC_Info[index][1])
        time.sleep(0.1)

        win32api.SendMessage(hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
        win32api.SendMessage(hwnd, win32con.WM_KEYUP, win32con.VK_RETURN, 0)

    def _place_client_window(self, index, hwnd, nbr_monitor):
        if not hwnd or not win32gui.IsWindow(hwnd):
            return

        if index == 0 and ((self.NBR_ACCOUNT <= GROUP_SIZE and nbr_monitor >= 2) or self.NBR_ACCOUNT == 1):
            win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
            return

        if index < len(self.listCoord):
            x, y, w, h = self.listCoord[index]
            win32gui.MoveWindow(hwnd, x, y, w, h, True)

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
            return

        self.script_running = True
        self.serverthread.sendAllClients(b"Bot: ON")
        self.ScriptOnOff_Label.config(text="ON", foreground="green")

    # =========================
    # Specs / rôles optimisés
    # =========================
    def check_all_specs(self):
        self.spec_check_after_id = None
        roles_changed = False

        for i in range(min(self.NBR_ACCOUNT, len(self.serverthread.clients_thread))):
            ct = self.serverthread.get_client_thread(i)

            if ct is None or not ct.running or len(ct.Name) <= 0:
                continue

            spec = self.SpecialisationList[i].get()

            if ct.currentSpec == spec:
                continue

            ct.currentSpec = spec
            self.send_spec_to_client(i, ct, spec)
            roles_changed = True

        if roles_changed:
            self.broadcast_roles()

        self.spec_check_after_id = self.after(500, self.check_all_specs)

    def send_spec_to_client(self, index, client, spec):
        for option_index, option in enumerate(self.OptionList[index]):
            if spec != option:
                continue

            try:
                client.conn.sendall(f"Spec: {option_index} ".encode("utf-8"))
            except Exception:
                client.running = False

            return

    def broadcast_roles(self, force=False):
        for i in range(min(self.NBR_ACCOUNT, len(self.serverthread.clients_thread))):
            ct = self.serverthread.get_client_thread(i)

            if ct is None or not ct.running or len(ct.Name) <= 0:
                continue

            role_nbr = getRole(ct.Class, ct.currentSpec)
            payload = (role_nbr, ct.Name)

            if not force and self.role_cache.get(i) == payload:
                continue

            self.role_cache[i] = payload

            msg = f"Role{role_nbr}_{str(i).zfill(2)}_{str(len(ct.Name)).zfill(2)}_{ct.Name}"
            self.serverthread.sendAllClients(msg.encode("utf-8"))

    # =========================
    # UI update joueur
    # =========================
    def update_player_class(self, index, name, player_class, options, color, faction):
        if index >= MAX_ACCOUNTS:
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
        self.role_cache.pop(index, None)
        self.schedule_player_refresh()
        self.broadcast_roles(force=True)

    def clear_player(self, index):
        if index >= MAX_ACCOUNTS:
            return

        self.PlayerFaction[index] = -1
        self.role_cache.pop(index, None)

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
        self.schedule_player_refresh()


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

        with interface.serverthread.lock:
            if self.index < len(interface.serverthread.clients):
                interface.serverthread.clients[self.index] = None

            if self.index < len(interface.serverthread.clients_thread):
                interface.serverthread.clients_thread[self.index] = None

        try:
            self.conn.close()
        except Exception:
            pass

        try:
            if self.index < len(interface.hwndACC) and win32gui.IsWindow(interface.hwndACC[self.index]):
                win32api.SendMessage(interface.hwndACC[self.index], win32con.WM_CLOSE, 0, 0)
        except Exception:
            pass

        interface.after(0, lambda index=self.index: interface.clear_player(index))

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

        name_len = str(len(self.Name)).zfill(2)
        player_id = str(self.index).zfill(2)

        out0 = f"Craft0{player_id}{name_len}{self.CraftSkill[0]}{self.Name}"
        out1 = f"Craft1{player_id}{name_len}{self.CraftSkill[1]}{self.Name}"

        interface.serverthread.sendAllClients(out0.encode("utf-8"))
        interface.serverthread.sendAllClients(out1.encode("utf-8"))


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

        self.lock = threading.Lock()
        self.reserved_indices = []

    def reserve_index(self, index):
        if index < 0 or index >= MAX_ACCOUNTS:
            return

        with self.lock:
            if index not in self.reserved_indices:
                self.reserved_indices.append(index)

            while len(self.clients) <= index:
                self.clients.append(None)

            while len(self.clients_thread) <= index:
                self.clients_thread.append(None)

    def get_client_thread(self, index):
        with self.lock:
            if index < len(self.clients_thread):
                return self.clients_thread[index]
            return None

    def _take_accept_index(self):
        with self.lock:
            if self.reserved_indices:
                return self.reserved_indices.pop(0)

            free_index = next((i for i, c in enumerate(self.clients) if c is None), None)

            if free_index is not None:
                return free_index

            if len(self.clients) >= MAX_ACCOUNTS:
                return None

            self.clients.append(None)
            self.clients_thread.append(None)

            return len(self.clients) - 1

    def run(self):
        self.tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.tcp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.tcp_socket.bind((SERVER_HOST, SERVER_PORT))
        self.tcp_socket.listen(MAX_ACCOUNTS)

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

            free_index = self._take_accept_index()

            if free_index is None:
                try:
                    conn.close()
                except Exception:
                    pass
                continue

            with self.lock:
                while len(self.clients) <= free_index:
                    self.clients.append(None)

                while len(self.clients_thread) <= free_index:
                    self.clients_thread.append(None)

                self.clients[free_index] = (conn, addr)

            ct = client_thread(free_index, conn, addr)

            with self.lock:
                self.clients_thread[free_index] = ct

            ct.start()

            print(f"[+] New client {free_index + 1}: {addr[0]}:{addr[1]}")

        with self.lock:
            threads = list(self.clients_thread)

        for ct in threads:
            if ct is None:
                continue

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

        try:
            interface.after(0, interface.destroy)
        except Exception:
            pass

    def _safe_send(self, client_tuple, msg):
        if client_tuple is None:
            return False

        try:
            if not msg.endswith(b"\n"):
                msg += b"\n"

            client_tuple[0].sendall(msg)
            return True
        except Exception:
            return False

    def sendAllClients(self, msg):
        with self.lock:
            snapshot = list(enumerate(self.clients))

        for i, client in snapshot:
            if client is not None and not self._safe_send(client, msg):
                with self.lock:
                    if i < len(self.clients):
                        self.clients[i] = None

    def sendGroupClients(self, msg, index=-1):
        if interface.NBR_ACCOUNT <= 0:
            return

        with self.lock:
            clients_snapshot = list(self.clients)

        if not clients_snapshot:
            return

        if index == -1:
            foreground = win32gui.GetForegroundWindow()

            for group_index in range(((interface.NBR_ACCOUNT - 1) // GROUP_SIZE) + 1):
                start = group_index * GROUP_SIZE
                end = min(start + GROUP_SIZE, interface.NBR_ACCOUNT)

                if foreground not in interface.hwndACC[start:end]:
                    continue

                for y in range(start, end):
                    if y < len(clients_snapshot) and clients_snapshot[y] is not None:
                        if not self._safe_send(clients_snapshot[y], msg):
                            with self.lock:
                                if y < len(self.clients):
                                    self.clients[y] = None

                return

        start = index - (index % GROUP_SIZE)
        end = min(start + GROUP_SIZE, interface.NBR_ACCOUNT)

        for y in range(start, end):
            if y < len(clients_snapshot) and clients_snapshot[y] is not None:
                if not self._safe_send(clients_snapshot[y], msg):
                    with self.lock:
                        if y < len(self.clients):
                            self.clients[y] = None

    def sendMainClients(self, msg):
        if interface.NBR_ACCOUNT <= 0:
            return

        with self.lock:
            clients_snapshot = list(self.clients)

        if not clients_snapshot:
            return

        for idx in range(0, interface.NBR_ACCOUNT, GROUP_SIZE):
            if idx >= len(clients_snapshot):
                continue

            if clients_snapshot[idx] is None:
                continue

            if not self._safe_send(clients_snapshot[idx], msg):
                with self.lock:
                    if idx < len(self.clients):
                        self.clients[idx] = None

    def sendMainClient(self, msg):
        self.sendMainClients(msg)


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