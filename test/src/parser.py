import os

def init_config(file_path):
    if not os.path.isfile(file_path):
        with open(file_path, 'w') as file:
            file.write("PATH_WoW=\nPATH_Screenshot=\nACC_Infos=[")
            for i in range(25):
                file.write("('','')")
                if(i+1 < 25): file.write(", ")
            file.write("]")

def get_value(file_path, info, start):
    with open(file_path, 'r') as file:
        for line in file:
            if(line.find(info) >= 0):
                start_pos = line.find(start)
                if(start_pos >= 0):
                    end_pos = line.find('\n')
                    if(end_pos >= 0): tmp = line[start_pos+1:end_pos]
                    else: tmp = line[start_pos+1::]
                    if(tmp != "\n"): return tmp
    return ''
    
def get_multiplevalues(file_path, info):
    tmpTab = []
    with open(file_path, 'r') as file:
        for line in file:
            if(line.find(info) >= 0 and line.find('(') >= 0 and line.find(')') >= 0):
                while(line.find('(') >= 0 and line.find(')') >= 0):
                    start_pos = line.find('(')
                    bet_pos = line.find(',')
                    end_pos = line.find(')')
                    tmp = line[start_pos+2:bet_pos-1]
                    tmp2 = line[bet_pos+2:end_pos-1]
                    if(tmp2 != "\n"):
                        tmpTab.append((tmp,tmp2))
                    line = line[end_pos+2::]
                return tmpTab
    for i in range(25): tmpTab.append(('', ''))
    return tmpTab
    
def modify_config(file_path, path_wow='', keybind_info='', acc_info=''):
    if(path_wow == ''): path_wow = get_value('config.conf', 'PATH_WoW', '=')
    if(keybind_info == ''): keybind_info = get_multiplevalues('config.conf', 'KEYBIND_Infos')
    if(acc_info == ''): acc_info = get_multiplevalues('config.conf', 'ACC_Infos')
    with open(file_path, 'w') as file:
        file.write("PATH_WoW="+path_wow+"\nKEYBIND_Infos=[")
        for i in range(len(keybind_info)):
            file.write("('"+keybind_info[i][0]+"','"+keybind_info[i][1]+"')")
            if(i+1 < len(keybind_info)): file.write(", ")
        file.write("]\nACC_Infos=[")
        for i in range(len(acc_info)):
            file.write("('"+acc_info[i][0]+"','"+acc_info[i][1]+"')")
            if(i+1 < len(acc_info)): file.write(", ")
        file.write("]")