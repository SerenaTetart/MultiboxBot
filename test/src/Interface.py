import os
import sys
import time
import socket
import threading
import subprocess

from PIL import ImageTk, Image, ImageGrab
from tkinter import messagebox, filedialog
from pynput import keyboard
from tkinter import ttk
import tkinter as tk
import mouse
import PIL

import atexit, signal
import win32gui, win32con, win32api, win32job, win32process #pywin32

import parser

class Interface(tk.Tk):
    def __init__(self):
        super().__init__()
        
         # Window
        self.resizable(False,False)
        self.title('Multibox')
        self.protocol("WM_DELETE_WINDOW", self.quit_program)
        self.iconphoto(True, ImageTk.PhotoImage(Image.open(r"assets/icon.jpg")))
        self.iconWoW = tk.PhotoImage(file='assets/iconWoW.png')
        self.iconKeybind = tk.PhotoImage(file='assets/iconKeybind.png')
        self.attributes('-topmost', True)
        
         # Variables
        parser.init_config('config.conf')
        self.PATH_WoW = parser.get_value('config.conf', 'PATH_WoW', '=')
        self.ACC_Info = parser.get_multiplevalues('config.conf', 'ACC_Infos')
        self.KEYBIND_Info = parser.get_multiplevalues('config.conf', 'KEYBIND_Infos')
        self.MOVEMENT_KEY = [win32con.VK_RIGHT, win32con.VK_UP, win32con.VK_DOWN, win32con.VK_LEFT]
        
        self.listCoord = []
        self.hwndACC = [] #Get windows handle
        self.script_running = False
        self.InMovement = [0 for x in range(20)]
        self.indexIMG = 0
                
        self.serverthread = server_thread()
        self.serverthread.start()
        
         # Tabs
        tabControl = ttk.Notebook(self)
        tab1 = ttk.Frame(tabControl)
        tab2 = ttk.Frame(tabControl)
        tabControl.pack(expand = 1, fill ="both")
        tabControl.add(tab1, text='Menu')
        tabControl.add(tab2, text='Options')
        
         # Widgets
        self.NBR_ACCOUNT = 0
        self.WoWDirButton = tk.Button(tab2, image=self.iconWoW, command=lambda: self.selectWoWDir())
        self.WoWDirEntry = tk.Entry(tab2, state='normal', width = 39)
        self.WoWDirEntry.insert(0,self.PATH_WoW)
        self.WoWDirEntry.configure(state='disabled')
        self.ModifyCredentials_Button = tk.Button(tab2, text='Modify credentials', command=lambda: self.open_credentials_tab(), padx=5, pady=5)
        self.ModifyKeyBindings_Button = tk.Button(tab2, text='Modify key bindings', command=lambda: self.open_keybindings_tab(), padx=5, pady=5)
        self.LaunchRepair_Button = tk.Button(tab1, text='Launch', command=lambda: self.launch_repair_clients(), padx=5, pady=5)
        self.ScriptOnOff_Label = tk.Label(tab1, text="OFF", foreground='red')
        self.NbrClient_Entry = tk.Entry(tab1, state='normal', justify=tk.CENTER, width = 7)
        self.NbrClient_Label = tk.Label(tab1, text="Number clients:")
        self.MCNoAuto = tk.IntVar()
        self.MCNoAuto_CheckBtn = tk.Checkbutton(tab1, text = "MC: no auto", variable = self.MCNoAuto, command=lambda: (self.sendCheckbox("1"+str(self.MCNoAuto.get())), self.MCAutoMove.set(0), self.sendCheckbox("2"+str(self.MCAutoMove.get()))), onvalue = 1, offvalue = 0, height=2, width = 16)
        self.MCAutoMove = tk.IntVar()
        self.MCAutoMove_CheckBtn = tk.Checkbutton(tab1, text = "MC: auto focus/move", variable = self.MCAutoMove, command=lambda: (self.sendCheckbox("2"+str(self.MCAutoMove.get())), self.MCNoAuto.set(0), self.sendCheckbox("1"+str(self.MCNoAuto.get()))), onvalue = 1, offvalue = 0, height=2, width = 16)
        
         # Players and informations related
        self.Group_Label = tk.Label(tab1, text="Players detected:")
        self.Name_Label = []; self.Class_Label = []; self.SpecialisationList = []
        self.Specialisation_Menu = []; self.OptionList = []
        for i in range(25):
            self.Name_Label.append(tk.Label(tab1, text="Null", foreground='grey'))
            self.Class_Label.append(tk.Label(tab1, text="Null", foreground='grey'))
            self.SpecialisationList.append(tk.StringVar(self))
            self.OptionList.append(['Null'])
            self.SpecialisationList[i].set(self.OptionList[i][0])
            self.Specialisation_Menu.append(tk.OptionMenu(tab1, self.SpecialisationList[i], *self.OptionList[i]))
            self.Specialisation_Menu[i].config(width = 11)
        
         # Config
        self.LaunchRepair_Button.config(width = 6)
        
         # Grid
        self.WoWDirButton.grid(row=0, column=0, sticky=tk.E, padx=2, pady=10)
        self.WoWDirEntry.grid(row=0, column=1, columnspan=6, padx=2)
        self.ModifyCredentials_Button.grid(row=1, column=0, columnspan=3)
        self.ModifyKeyBindings_Button.grid(row=1, column=4, columnspan=3)
        self.ScriptOnOff_Label.grid(row=0, column=5, sticky=tk.E)
        self.LaunchRepair_Button.grid(row=1, column=0, columnspan=2, pady=2)
        self.NbrClient_Label.grid(row=1, column=3, columnspan=2, sticky=tk.E)
        self.NbrClient_Entry.grid(row=1, column=5, sticky=tk.W)
        self.MCNoAuto_CheckBtn.grid(row=2, column=0, columnspan=3)
        self.MCAutoMove_CheckBtn.grid(row=2, column=3, columnspan=3)
        
    def show_infoAccounts(self, nbr):
        y = 0
        self.Group_Label.grid(row=3, column=2, columnspan=3, pady=10)
        for i in range(nbr):
            if(i%2 == 0):
                self.Name_Label[i].grid(row=4+y, column=0)
                self.Class_Label[i].grid(row=4+y, column=1)
                self.Specialisation_Menu[i].grid(row=4+y, column=2)
            else:
                self.Name_Label[i].grid(row=4+y, column=3)
                self.Class_Label[i].grid(row=4+y, column=4)
                self.Specialisation_Menu[i].grid(row=4+y, column=5)
                y = y+1
        self.WoWDirEntry.configure(width=60)
        
    def sendCheckbox(self, msg):
        msg = "C"+msg
        self.serverthread.sendAllClients(bytes(msg, 'utf-8'))
        
    def quit_program(self):
        #Disconnect clients
        self.serverthread.sendAllClients(b"QUIT")
        self.script_running = False
        listener.stop()
        for hwnd in self.hwndACC:
            if(win32gui.IsWindow(hwnd)):
                win32api.SendMessage(hwnd, win32con.WM_CLOSE, 0, 0)
        self.serverthread.running = False
        tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp_socket.connect(('localhost', 50001))
        tcp_socket.close()
        sys.exit(0)
        
    def selectWoWDir(self):
        tmp = filedialog.askopenfile(title='Select your wow vanilla client', filetypes=[('WoW', ['exe'])], initialdir=self.PATH_WoW)
        if(tmp != None and tmp.name != self.PATH_WoW):
            parser.modify_config('config.conf', path_wow=tmp.name)
            self.PATH_WoW = tmp.name
            self.WoWDirEntry.configure(state='normal')
            self.WoWDirEntry.delete(0,tk.END)
            self.WoWDirEntry.insert(0,tmp.name)
            self.WoWDirEntry.configure(state='disabled')
        
    def open_credentials_tab(self):
        global credentialTab
        global credentials_Entry
        try:
            if(credentialTab.state() == "normal"): credentialTab.focus()
        except:
            credentialTab = tk.Toplevel(self)
            credentialTab.title('Credentials')
            credentialTab.resizable(False,False)
            credentialTab.attributes('-topmost', True)
            account_Label = []; username_Label = []
            password_Label = []; credentials_Entry = []
            validate_Button = tk.Button(credentialTab, text='Validate changes', command=lambda: self.modify_crendentials())
            y = 0
            for i in range(25):
                 # Widgets
                account_Label.append(tk.Label(credentialTab, text="Account "+str(i+1)))
                username_Label.append(tk.Label(credentialTab, text="Username:"))
                password_Label.append(tk.Label(credentialTab, text="Password:"))
                credentials_Entry.append([tk.Entry(credentialTab, width = 15), tk.Entry(credentialTab, width = 15)])
                credentials_Entry[i][0].insert(0, self.ACC_Info[i][0])
                credentials_Entry[i][1].insert(0, self.ACC_Info[i][1])
                # Grid
                col = 0
                if(i >= 20): col = 4
                elif(i >= 10): col = 2
                if(i == 20 or i == 10): y = 0
                account_Label[i].grid(row=y, column=col+1)
                username_Label[i].grid(row=y+1, column=col)
                password_Label[i].grid(row=y+2, column=col)
                credentials_Entry[i][0].grid(row=y+1, column=col+1)
                credentials_Entry[i][1].grid(row=y+2, column=col+1)
                y = y+3
            validate_Button.grid(row=28, column=5, padx=2)
        
    def modify_crendentials(self):
        for i in range(25):
            self.ACC_Info[i] = (credentials_Entry[i][0].get(), credentials_Entry[i][1].get())
        parser.modify_config('config.conf', acc_info=self.ACC_Info)
        credentialTab.destroy()
        
    #==================================================#
        
    def open_keybindings_tab(self):
        global keybindingsTab
        global keybind_Entry
        try:
            if(keybindingsTab.state() == "normal"): keybindingsTab.focus()
        except:
            keybindingsTab = tk.Toplevel(self)
            keybindingsTab.title('Credentials')
            keybindingsTab.resizable(False,False)
            keybindingsTab.attributes('-topmost', True)
            keybind_Label = []; keybind_Entry = []; keybind_Button = []
            listLabel = ["Use Hearthstone:", "Use Mount:", "Toggle auto-learn:", "Group passive/aggressive:"]
            for i in range(len(listLabel)):
                keybind_Label.append(tk.Label(keybindingsTab, text=listLabel[i]))
                keybind_Entry.append(tk.Entry(keybindingsTab, state='normal', width = 15))
                keybind_Entry[i].delete(0,tk.END)
                keybind_Entry[i].insert(0, self.KEYBIND_Info[i][1])
                keybind_Entry[i].configure(state='disabled')
                keybind_Button.append(tk.Button(keybindingsTab, image=self.iconKeybind, command=lambda i=i: self.bindKey(i)))
                keybind_Label[i].grid(row=i, column=0)
                keybind_Entry[i].grid(row=i, column=1)
                keybind_Button[i].grid(row=i, column=2)
        
    def bindKey(self, index):
        if(index == 0): keybindingsTab.bind("<Key>", self.key_pressed_Hearthstone)
        elif(index == 1): keybindingsTab.bind("<Key>", self.key_pressed_Mount)
        elif(index == 2): keybindingsTab.bind("<Key>", self.key_pressed_AutoLearn)
        elif(index == 3): keybindingsTab.bind("<Key>", self.key_pressed_PA)
        
    def key_pressed_Hearthstone(self, event):
        keybindingsTab.unbind("<Key>")
        keybind_Entry[0].configure(state='normal')
        keybind_Entry[0].delete(0,tk.END)
        keybind_Entry[0].insert(0, event.keysym)
        keybind_Entry[0].configure(state='disabled')
        self.KEYBIND_Info[0] = (str(event.keycode), event.keysym)
        parser.modify_config('config.conf', keybind_info=self.KEYBIND_Info)
        
    def key_pressed_Mount(self, event):
        keybindingsTab.unbind("<Key>")
        keybind_Entry[1].configure(state='normal')
        keybind_Entry[1].delete(0,tk.END)
        keybind_Entry[1].insert(0, event.keysym)
        keybind_Entry[1].configure(state='disabled')
        self.KEYBIND_Info[1] = (str(event.keycode), event.keysym)
        parser.modify_config('config.conf', keybind_info=self.KEYBIND_Info)
        
    def key_pressed_AutoLearn(self, event):
        keybindingsTab.unbind("<Key>")
        keybind_Entry[2].configure(state='normal')
        keybind_Entry[2].delete(0,tk.END)
        keybind_Entry[2].insert(0, event.keysym)
        keybind_Entry[2].configure(state='disabled')
        self.KEYBIND_Info[2] = (str(event.keycode), event.keysym)
        parser.modify_config('config.conf', keybind_info=self.KEYBIND_Info)
        
    def key_pressed_PA(self, event):
        keybindingsTab.unbind("<Key>")
        keybind_Entry[3].configure(state='normal')
        keybind_Entry[3].delete(0,tk.END)
        keybind_Entry[3].insert(0, event.keysym)
        keybind_Entry[3].configure(state='disabled')
        self.KEYBIND_Info[3] = (str(event.keycode), event.keysym)
        parser.modify_config('config.conf', keybind_info=self.KEYBIND_Info)
        
    #==================================================#
        
    def send_client_txt(self, hwnd, txt):
        #Send text to window
        for c in txt:
            win32api.SendMessage(hwnd, win32con.WM_CHAR, ord(c), 0)
    
    def on_KeyPress(self, key):
        try:
            key_code = key.vk
        except AttributeError:
            key_code = key.value.vk
        for i in range(len(self.KEYBIND_Info)):
            if(str(key_code) == self.KEYBIND_Info[i][0]):
                msg = "K"+str(i+1)
                self.serverthread.sendGroupClients(bytes(msg, 'utf-8'))
                break

    def adapt_listCoord(self):
        monitor = win32api.EnumDisplayMonitors()
        idx_primary = next(
            i for i, m in enumerate(monitor)
            if win32api.GetMonitorInfo(m[0])["Flags"] & win32con.MONITORINFOF_PRIMARY
        )
        x_gap = 8; width_increase = 18; height_increase = 23
        if (len(monitor) == 1):
            monitor_x0 = monitor[0][2][0]; monitor_y0 = monitor[0][2][1]
            monitor_width = monitor[0][2][2] - monitor_x0
            monitor_height = monitor[0][2][3] - monitor_y0
            nbr_bottom = self.NBR_ACCOUNT // 2
            nbr_top = self.NBR_ACCOUNT - nbr_bottom
            top_nbr = 0; bottom_nbr = 0
            tmp = True
            for i in range(self.NBR_ACCOUNT):
                if(tmp == True):
                    x = monitor_x0 + ((monitor_width // nbr_top) * top_nbr) - x_gap
                    width = (monitor_width // nbr_top)+width_increase
                    height = monitor_height
                    if (nbr_bottom > 0): height = (monitor_height // 2)
                    self.listCoord.append((x, 0, width, height+height_increase))
                    top_nbr += 1
                    tmp = False
                else:
                    x = monitor_x0 + ((monitor_width // nbr_bottom) * bottom_nbr) - x_gap
                    y = 0
                    if(nbr_principal > 1): y = monitor_height // 2
                    width = (monitor_width // nbr_bottom)+width_increase
                    height = (monitor_height // 2)
                    self.listCoord.append((x, y-15, width, height+height_increase))
                    bottom_nbr += 1
                    tmp = True
        elif (len(monitor) >= 2):
            main_monitor = idx_primary; second_monitor = 1
            monitor1_x0 = monitor[main_monitor][2][0]; monitor1_y0 = monitor[main_monitor][2][1]
            monitor1_x1 = monitor[main_monitor][2][2]; monitor1_y1 = monitor[main_monitor][2][3]
            monitor1_width = monitor1_x1 - monitor1_x0; monitor1_height = monitor1_y1 - monitor1_y0
            monitor2_x0 = monitor[second_monitor][2][0]; monitor2_y0 = monitor[second_monitor][2][1]
            monitor2_x1 = monitor[second_monitor][2][2]; monitor2_y1 = monitor[second_monitor][2][3]
            #print(monitor1_x0, monitor1_x1, monitor1_y0, monitor1_y1)
            #print(monitor2_x0, monitor2_x1, monitor2_y0, monitor2_y1)
            monitor2_width = monitor2_x1 - monitor2_x0; monitor2_height = monitor2_y1 - monitor2_y0
            nbr_principal = ((self.NBR_ACCOUNT - 1) // 5) + 1
            nbr_second = self.NBR_ACCOUNT - nbr_principal
            nbr_bottom = [nbr_principal // 2, nbr_second // 2]
            nbr_top = [nbr_principal - nbr_bottom[0], nbr_second - nbr_bottom[1]]
            top_nbr = [0, 0]; bottom_nbr = [0, 0]
            tmp = True; tmp2 = True
            for i in range(self.NBR_ACCOUNT):
                if(i % 5 == 0):
                    if(tmp == True):
                        x = monitor1_x0 + ((monitor1_width // nbr_top[0]) * top_nbr[0]) - x_gap
                        width = (monitor1_width // nbr_top[0])+width_increase
                        height = monitor1_height
                        if (nbr_bottom[0] > 0): height = (monitor1_height // 2)
                        self.listCoord.append((x, 0, width, height+height_increase))
                        top_nbr[0] += 1
                        tmp = False
                    else:
                        x = monitor1_x0 + ((monitor1_width // nbr_bottom[0]) * bottom_nbr[0]) - x_gap
                        y = 0
                        if(nbr_principal > 1): y = monitor1_height // 2
                        width = (monitor1_width // nbr_bottom[0])+width_increase
                        height = (monitor1_height // 2)
                        self.listCoord.append((x, y-height_increase+10, width, height+height_increase))
                        bottom_nbr[0] += 1
                        tmp = True
                elif(tmp2 == True):
                    x = monitor2_x0 + ((monitor2_width // nbr_top[1]) * top_nbr[1]) - x_gap
                    width = (monitor2_width // nbr_top[1])+width_increase
                    height = monitor2_height
                    if (nbr_bottom[1] > 0): height = (monitor2_height // 2)
                    self.listCoord.append((x, monitor2_y0, width, height+height_increase)) #-40
                    top_nbr[1] += 1
                    tmp2 = False
                else:
                    x = monitor2_x0 + ((monitor2_width // nbr_bottom[1]) * bottom_nbr[1]) - x_gap
                    y = monitor2_height // 2 + monitor2_y0
                    width = (monitor2_width // nbr_bottom[1])+width_increase
                    height = (monitor2_height // 2)
                    self.listCoord.append((x, y-height_increase+10, width, height+height_increase)) #-30
                    bottom_nbr[1] += 1
                    tmp2 = True
        
    def launch_repair_clients(self):
        #Launch or Repair clients
        nbr_monitor = win32api.GetSystemMetrics(win32con.SM_CMONITORS)
        if self.LaunchRepair_Button.config('text')[-1] == 'Launch':
            if(self.NbrClient_Entry.get() == ""): messagebox.showerror('Error', 'You must specify the number of clients !')
            elif (int(self.NbrClient_Entry.get()) <= 0): messagebox.showerror('Error', 'You can\'t have ' + str(self.NBR_ACCOUNT) + ' clients !')
            else:
                self.NBR_ACCOUNT = int(self.NbrClient_Entry.get())
                self.NbrClient_Entry.configure(state='disabled')
                self.LaunchRepair_Button.config(text='Repair')
                self.adapt_listCoord()
                for i in range(self.NBR_ACCOUNT):
                    p1 = subprocess.Popen(["./src/Bootstrap.exe", self.PATH_WoW])
                    p1.wait()
                    hwnd = win32gui.FindWindow(None, "World of Warcraft")
                    while hwnd == 0:
                        hwnd = win32gui.FindWindow(None, "World of Warcraft")
                        time.sleep(0.1)
                    time.sleep(0.5)
                    win32gui.SetWindowText(hwnd, "WoW"+str(i+1))
                    self.hwndACC.append(hwnd)
                    if(i == 0 and ((self.NBR_ACCOUNT <= 5 and nbr_monitor >= 2) or (self.NBR_ACCOUNT == 1))): win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
                    else:
                        win32gui.MoveWindow(hwnd, self.listCoord[i][0], self.listCoord[i][1], self.listCoord[i][2], self.listCoord[i][3], True)
                        if(i == 1 and self.NBR_ACCOUNT == 2 and nbr_monitor == 2): win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
                for i in range(self.NBR_ACCOUNT): #Enter username/password
                    self.send_client_txt(self.hwndACC[i], self.ACC_Info[i][0])
                    win32api.SendMessage(self.hwndACC[i], win32con.WM_KEYDOWN, win32con.VK_TAB, 0)
                    win32api.SendMessage(self.hwndACC[i], win32con.WM_KEYUP, win32con.VK_TAB, 0)
                    self.send_client_txt(self.hwndACC[i], self.ACC_Info[i][1])
                    time.sleep(0.1)
                    win32api.SendMessage(self.hwndACC[i], win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
                    win32api.SendMessage(self.hwndACC[i], win32con.WM_KEYUP, win32con.VK_RETURN, 0)
                self.show_infoAccounts(self.NBR_ACCOUNT)
        else: #Repair
            for i in range(self.NBR_ACCOUNT):
                if(not win32gui.IsWindow(self.hwndACC[i])):
                    p1 = subprocess.Popen(["./src/Bootstrap.exe", self.PATH_WoW])
                    p1.wait()
                    hwnd = win32gui.FindWindow(None, "World of Warcraft")
                    while hwnd == 0:
                        hwnd = win32gui.FindWindow(None, "World of Warcraft")
                        time.sleep(0.1)
                    time.sleep(0.5)
                    win32gui.SetWindowText(hwnd, "WoW"+str(i+1))
                    self.hwndACC[i] = hwnd
                    self.send_client_txt(self.hwndACC[i], self.ACC_Info[i][0])
                    win32api.SendMessage(self.hwndACC[i], win32con.WM_KEYDOWN, win32con.VK_TAB, 0)
                    win32api.SendMessage(self.hwndACC[i], win32con.WM_KEYUP, win32con.VK_TAB, 0)
                    self.send_client_txt(self.hwndACC[i], self.ACC_Info[i][1])
                    time.sleep(0.1)
                    win32api.SendMessage(self.hwndACC[i], win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
                    win32api.SendMessage(self.hwndACC[i], win32con.WM_KEYUP, win32con.VK_RETURN, 0)
                    if(i == 0 and ((self.NBR_ACCOUNT <= 5 and nbr_monitor >= 2) or (self.NBR_ACCOUNT == 1))): win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
                    else:
                        win32gui.MoveWindow(hwnd, self.listCoord[i][0], self.listCoord[i][1], self.listCoord[i][2], self.listCoord[i][3], True)
                        if(i == 1 and self.NBR_ACCOUNT == 2 and nbr_monitor == 2): win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
                else:
                    hwnd = win32gui.FindWindow(None, "WoW"+str(i+1))
                    if(i == 0 and ((self.NBR_ACCOUNT <= 5 and nbr_monitor >= 2) or (self.NBR_ACCOUNT == 1))): win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
                    else:
                        win32gui.MoveWindow(hwnd, self.listCoord[i][0], self.listCoord[i][1], self.listCoord[i][2], self.listCoord[i][3], True)
                        if(i == 1 and self.NBR_ACCOUNT == 2 and nbr_monitor == 2): win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
            
    def activateBot(self):
        self.MOVEMENT_KEY = [win32con.VK_RIGHT, win32con.VK_UP, win32con.VK_DOWN, win32con.VK_LEFT]
        if(self.script_running):
            self.script_running = False
            self.serverthread.sendAllClients(b"Bot: OFF")
            self.ScriptOnOff_Label.config(text='OFF')
            self.ScriptOnOff_Label.config(foreground='red')
        else:
            self.script_running = True
            self.serverthread.sendAllClients(b"Bot: ON")
            self.ScriptOnOff_Label.config(text='ON')
            self.ScriptOnOff_Label.config(foreground='green')
        

def rgb_hack(rgb):
    return "#%02x%02x%02x" % rgb 
    
def getRole(Class, Spec):
    if(Spec == "Protection" or Spec == "Feral (bear)"): return 0
    elif(Class == "Warrior" or Class == "Rogue" or (Class == "Druid" and Spec == "Feral (cat)") or (Class == "Paladin" and Spec != "Holy") or (Class == "Shaman" and Spec == "Enhancement")): return 1
    elif(Spec == "Restoration" or Spec == "Holy" or (Class == "Priest" and Spec != "Shadow")): return 3
    else: return 2

class client_thread(threading.Thread):
    def __init__(self, index, conn, addr):
        super(client_thread, self).__init__()
        self.running = True
        self.conn = conn
        self.addr = addr
        self.index = index
        self.currentSpec = "Null"
        self.Name = ""
        self.Class = ""
        self.CraftSkill = [0, 0]
       
    def run(self):
        interface.after(500, self.checkSpecChange)
        while(self.running):
            try:
                data = self.conn.recv(128)
                if(data):
                    data = data.decode('utf-8')
                    if(data.find("Class") > -1):
                        ind = data.find("Class")
                        self.Name = data[5:ind]
                        self.Class = data[ind+6::]
                        interface.Name_Label[self.index].config(text=self.Name)
                        interface.Class_Label[self.index].config(text=self.Class)
                        if(self.Class == "Druid"):
                            interface.OptionList[self.index] = ['Balance', 'Feral (bear)', 'Feral (cat)', 'Restoration']
                            color = "orange"
                        elif(self.Class == "Hunter"):
                            interface.OptionList[self.index] = ['Beast Mastery', 'Marksmanship', 'Survival']
                            color = "green"
                        elif(self.Class == "Mage"):
                            interface.OptionList[self.index] = ['Arcane', 'Fire', 'Frost']
                            color = rgb_hack((0, 210, 255))
                        elif(self.Class == "Paladin"):
                            interface.OptionList[self.index] = ['Holy', 'Protection', 'Retribution']
                            color = rgb_hack((255, 0, 122))
                        elif(self.Class == "Priest"):
                            interface.OptionList[self.index] = ['Discipline', 'Holy', 'Shadow']
                            color = rgb_hack((210, 210, 210))
                        elif(self.Class == "Rogue"):
                            interface.OptionList[self.index] = ['Assassination', 'Combat', 'Subtlety']
                            color = rgb_hack((255, 210, 0))
                        elif(self.Class == "Shaman"):
                            interface.OptionList[self.index] = ['Elemental', 'Enhancement', 'Restoration']
                            color = "blue"
                        elif(self.Class == "Warlock"):
                            interface.OptionList[self.index] = ['Affliction', 'Demonology', 'Destruction']
                            color = "purple"
                        elif(self.Class == "Warrior"):
                            interface.OptionList[self.index] = ['Arms', 'Fury', 'Protection']
                            color = "brown"
                        elif(self.Class == "Null"):
                            interface.OptionList[self.index] = ['Null']
                            color = "grey"
                        interface.Specialisation_Menu[self.index]['menu'].delete(0, tk.END)
                        for option in interface.OptionList[self.index]:
                            interface.Specialisation_Menu[self.index]['menu'].add_command(label=option, command=tk._setit(interface.SpecialisationList[self.index], option))
                        if(self.Class != "Null"): interface.SpecialisationList[self.index].set(interface.OptionList[self.index][0])
                        else: interface.SpecialisationList[self.index].set(interface.OptionList[self.index][0])
                        interface.Name_Label[self.index].config(foreground="black")
                        interface.Class_Label[self.index].config(foreground=color)
                        self.currentSpec = 'Null'
                    elif(data.find("Craft") > -1):
                        self.CraftSkill[0] = int(data[5])
                        self.CraftSkill[1] = int(data[6])
                        msg = ('Craft0'+str(self.index).zfill(2)+str(len(self.Name)-1).zfill(2)+str(self.CraftSkill[0])+self.Name)
                        interface.serverthread.sendAllClients(bytes(msg, 'utf-8'))
                        msg = ('Craft1'+str(self.index).zfill(2)+str(len(self.Name)-1).zfill(2)+str(self.CraftSkill[1])+self.Name)
                        interface.serverthread.sendAllClients(bytes(msg, 'utf-8'))
                    #print("Client " + self.addr[0] + ":" + str(self.addr[1]) + " - message: " + data)
            except Exception as e:
                print(self.addr[0] + ":" + str(self.addr[1]) + " - " + str(e))
                interface.serverthread.clients[self.index] = 0
                self.running = False
                self.conn.close()
                if(win32gui.IsWindow(interface.hwndACC[self.index])):
                    win32api.SendMessage(interface.hwndACC[self.index], win32con.WM_CLOSE, 0, 0)
        print("[-] Client disconnected: " + self.addr[0] + ":" + str(self.addr[1]))
        
    def checkSpecChange(self):
        if(self.running):
            SpecTMP = interface.SpecialisationList[self.index].get()
            if(self.currentSpec != SpecTMP and len(self.Name) > 0):
                self.currentSpec = SpecTMP
                #Roles
                for i in range(0, interface.NBR_ACCOUNT):
                    if(len(interface.serverthread.clients_thread[i].Name) <= 0): continue
                    role_nbr = getRole(interface.serverthread.clients_thread[i].Class, interface.serverthread.clients_thread[i].currentSpec)
                    msg = (f"Role{role_nbr}_"+str(i).zfill(2)+'_'+str(len(interface.serverthread.clients_thread[i].Name)-1).zfill(2)+'_'+interface.serverthread.clients_thread[i].Name)
                    interface.serverthread.sendAllClients(bytes(msg, 'utf-8'))
                #Specialisation
                for i in range(len(interface.OptionList[self.index])):
                    if(self.currentSpec == interface.OptionList[self.index][i]):
                        msg = ('Spec: '+str(i)+' ')
                        self.conn.send(bytes(msg, 'utf-8'))
                        time.sleep(0.01)
            interface.after(500, self.checkSpecChange)
        
class ChildServer:
    def __init__(self, exe_path):
        self.proc = subprocess.Popen([exe_path])
        # Create a job that kills all processes in it when the handle closes
        self.job = win32job.CreateJobObject(None, "")
        info = win32job.QueryInformationJobObject(self.job,
            win32job.JobObjectExtendedLimitInformation)
        info['BasicLimitInformation']['LimitFlags'] |= win32job.JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
        win32job.SetInformationJobObject(self.job,
            win32job.JobObjectExtendedLimitInformation, info)
        # Assign child to the job
        hProcess = win32api.OpenProcess(win32con.PROCESS_ALL_ACCESS, False, self.proc.pid)
        win32job.AssignProcessToJobObject(self.job, hProcess)

        # Ensure cleanup on exit/signals
        atexit.register(self._cleanup)
        signal.signal(signal.SIGINT,  lambda *_: self._cleanup(exit_code=130))
        signal.signal(signal.SIGTERM, lambda *_: self._cleanup(exit_code=143))

    def _cleanup(self, exit_code=None):
        if self.proc and self.proc.poll() is None:
            try:
                win32api.CloseHandle(self.job)
            except Exception:
                pass
            # As a last resort, ensure process is gone
            try:
                if self.proc.poll() is None:
                    self.proc.terminate()
            except Exception:
                pass
        if exit_code is not None:
            raise SystemExit(exit_code)
        
class server_thread(threading.Thread):
    def __init__(self):
        super(server_thread, self).__init__()
        self.running = True
        self.clients = []
        self.clients_thread = []

    def run(self):
        # Set up a TCP/IP server
        tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = ('localhost', 50001)
        tcp_socket.bind(server_address)
        tcp_socket.listen(25)
        while self.running:
            # blocking call, waits to accept a connection
            conn, addr = tcp_socket.accept()
            indextmp = -1
            for i in range(len(self.clients)):
                if(self.clients[i] == 0): indextmp = i
            if(indextmp == -1):
                self.clients.append((conn, addr))
                self.clients_thread.append(client_thread(len(self.clients)-1, conn, addr))
                self.clients_thread[len(self.clients_thread)-1].start()
            else:
                self.clients[indextmp] = (conn, addr)
                self.clients_thread[indextmp] = client_thread(indextmp, conn, addr)
                self.clients_thread[indextmp].start()
            print("[+] New client: " + addr[0] + ":" + str(addr[1]))
        for clientthread in self.clients_thread:
            clientthread.running = False
            clientthread.conn.close()
        print("server over...")
        interface.destroy()
        
    def sendAllClients(self, msg):
        for i in range(len(self.clients)):
            if(self.clients[i] != 0):
                self.clients[i][0].send(msg)
                time.sleep(0.01)
        
    def sendGroupClients(self, msg, index=-1):
        if(len(self.clients) > 0):
            if(index == -1):
                for i in range(((interface.NBR_ACCOUNT-1)//5)+1):
                    if(win32gui.GetForegroundWindow() in interface.hwndACC[i*5:(i*5)+((interface.NBR_ACCOUNT-1)%5)+1]):
                        for y in range(i*5, (i*5)+((interface.NBR_ACCOUNT-1)%5)+1):
                            if(self.clients[y] != 0):
                                self.clients[y][0].send(msg)
                                time.sleep(0.01)
                            """win32api.SendMessage(self.hwndACC[y], win32con.WM_KEYDOWN, key_code, 0)
                            win32api.SendMessage(self.hwndACC[y], win32con.WM_KEYUP, key_code, 0)"""
                        return
            else:
                for y in range(index-(index%5), (index-(index%5))+((interface.NBR_ACCOUNT-1)%5)+1):
                    if(self.clients[y] != 0):
                        self.clients[y][0].send(msg)
                        time.sleep(0.01)
                return
                
    #A REFAIRE POUR NE CIBLER QUE LES LEADER
    def sendTankClients(self, msg):
        if(len(self.clients) > 0):
            if(interface.NBR_ACCOUNT == 1):
                if(self.clients[0] != 0):
                    self.clients[0][0].send(msg)
                    time.sleep(0.01)
            else:
                for i in range(((interface.NBR_ACCOUNT-1)//5)+1):
                    if(self.clients[i*5] != 0):
                        self.clients[i*5][0].send(msg)
                        time.sleep(0.01)
        
    #Main :
if __name__== "__main__" :
    child = ChildServer(".\\src\\ServerNavigation.exe")
    interface = Interface()
    
    mouse.on_middle_click(interface.activateBot)

    listener = keyboard.Listener(on_press=interface.on_KeyPress)
    listener.start()

    interface.mainloop()
    
    print("\nYou can close this window...")
    sys.exit(0)