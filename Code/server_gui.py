#!/usr/bin/python3
# import tkinter 
# from tkinter import filedialog
# from tkinter import messagebox
# from tkinter import ttk
# import re
# import time
# import os
# import multiprocessing as mp
# import platform
# import datetime
import tkinter 
from tkinter import filedialog
from tkinter import messagebox
from tkinter import ttk
import re
from server_side import *


class Menubar(ttk.Frame):
    """Builds a menu bar for the top of the main window"""
    def __init__(self, parent, *args, **kwargs):
        ''' Constructor'''
        ttk.Frame.__init__(self, parent, *args, **kwargs)
        self.root = parent
        self.init_menubar()

    def on_exit(self):
        '''Exits program'''
        print("Exiting program...")
        quit()

    def display_help(self):
        # Displays help info in form of message box
        message_out = "Directions for use of this program\n"
        message_out += "Before using this program please run the realsense-viewer command in a terminal and make any desired setting changes to the depth camera as necessary and save them to a .json file.\n"
        #message_out += "You will see a list "
        # message_out += "Then, select the directory where you want the output to be saved.\nThen select the number of frames to process, in this case we are recording at 30 fps, so 30 frames is equivalent to 1 second of video.\n"
        # message_out += "The output will be a collection of .tif files in the directory you selected, each depth frame being numbered with its corresponding rgb frame.\n"
        # # Change font of message box
        #tkinter.Tk().option_add('*Dialog.msg.font', 'Helvetica 12')
        tkinter.messagebox.showinfo(title="Depth Realtime recording and encoding program", message=message_out)
        #tkinter.messagebox.showinfo(title="Decompression Program", message=message_out)

    def display_about(self):
        '''Displays info about program'''
        self.display_help()

    def init_menubar(self):
        self.menubar = tkinter.Menu(self.root)
        self.menu_file = tkinter.Menu(self.menubar) # Creates a "File" menu
        self.menu_file.add_command(label='Exit', command=self.on_exit) # Adds an option to the menu
        self.menubar.add_cascade(menu=self.menu_file, label='File') # Adds File menu to the bar. Can also be used to create submenus.

        self.menu_help = tkinter.Menu(self.menubar) #Creates a "Help" menu
        self.menu_help.add_command(label='Help', command=self.display_help)
        self.menu_help.add_command(label='About', command=self.display_about)
        self.menubar.add_cascade(menu=self.menu_help, label='Help')

        self.root.config(menu=self.menubar)


# The basic outline for this program is to have a main window with a menu bar at the top, and a frame below it that will contain the widgets for the program.
# We will need each of the following originally terminal commands to be in be in there own user
# Following widgets needed for each piece of functionality
#     - Option to fill in port number (default 5000) between 1025 and 65535
#     - Option to fill in the dimensions of the video stream (default 1280x720) (640x480, 1280x720, 1920x1080)
#     - Option to fill in the framerate of the video stream (default 30) (15, 30, 60, 90)
#     - Option to fill in the amount of time to record for in HH:MM:SS format (default 00:00:30)
#     - Option to fill in the CRF value (default 22) (0-51)
#     - Option to fill in the base name of the video files (default test_)
#     - Option to fill in the max depth of the depth camera (default 65535) (1-65535)
#     - Option to fill in the min depth of the depth camera (default 0) (0-65534)
#             Make sure that the max depth is greater than the min depth
#     - Option to fill in the depth unit of the depth camera (default 1000) (40-25000)
#     - Option to fill in the number of clients to connect to the server (default (size of the list of clients))
#     - Option to fill in the directory to save the video files to (default current directory) 
#     - Option to fill in the json file to be sent to the client and used for the video recordings (default empty string)
#     - Option to fill in the logging level (default error) (debug, info, error, warning, quiet)
# A start recording button will also be needed to start the recording process and a way of stopping all the recordings upon closing the program.

class ServerGUI(ttk.Frame):
    def __init__(self, parent, ip_lst, *args, **kwargs):
        ''' Constructor'''
        ttk.Frame.__init__(self, parent, *args, **kwargs)
        self.root = parent
        self.ip_lst = ip_lst
        self.init_server_gui()
        self.funct_to_call = funct_to_call

    def init_server_gui(self):
        # Set the title of the main window
        self.root.title("Depth Realtime recording and encoding program")
        # Set the size of the main window
        self.root.geometry("800x600")
        # Create a menu bar
        #self.root.geometry("600x400")
        self.grid(column=0, row=0, sticky='nsew')
        self.grid_columnconfigure(0, weight=1) # Allows column to stretch upon resizing
        self.grid_rowconfigure(0, weight=1) # Same with row
        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_rowconfigure(0, weight=1)
        self.root.option_add('*tearOff', 'FALSE') # Disables ability to tear menu bar into own window

        self.menubar = Menubar(self.root)
        self.dir_opt = {}
        #self.dir_opt['initialdir'] = '/home/'
        self.row_num = 1
        self.dir_opt['mustexist'] = True
        self.dir_opt['parent'] = self.root
        self.dir_opt['title'] = 'Choose a directory to save aligned color MP4, and depth MKV files.'

        self.json_opt = {}
        self.json_opt['defaultextension'] = '.json'
        self.json_opt['parent'] = self.root
        self.json_opt['title'] = 'Choose a JSON file to use for alignment.'
        self.json_opt['filetypes'] = [('JSON files', '.json')]
        
        # Create menubar
        self.menubar = Menubar(self.root)

        # Create a frame for the widgets
        # self = ttk.Frame(self.root)
        # self.pack(fill=tkinter.BOTH, expand=True)
        self.vid_save_dir = None
        self.json_file = None


        self.label_vid_save_dir = ttk.Label(self, text="Video save directory: ")
        self.btn_open_dir = ttk.Button(self, text='Save Directory', command=self.dir_dialog)

        self.label_json = ttk.Label(self, text="JSON file: ")
        self.btn_open_json = ttk.Button(self, text='Open JSON', command=self.json_dialog)

        self.btn_start = ttk.Button(self, text='Start Recording', command=self.start_recording)

        self.ir_label = ttk.Label(self, text="IR recording on or off: ")
        self.ir_var = tkinter.IntVar()
        self.ir_var.set(1)
        self.ir_on = ttk.Checkbutton(self, text="IR on", variable=self.ir_var, onvalue=1, offvalue=0)
        
        self.color_label = ttk.Label(self, text="Color recording on or off: ")
        self.color_var = tkinter.IntVar()
        self.color_var.set(1)
        self.color_on = ttk.Checkbutton(self, text="Color on", variable=self.color_var, onvalue=1, offvalue=0)

        self.align_label = ttk.Label(self, text="Align color and depth: ")
        self.align_var = tkinter.IntVar()
        self.align_var.set(1)
        self.align_on = ttk.Checkbutton(self, text="Align on", variable=self.align_var, onvalue=1, offvalue=0)

        # Create a label for the port number
        self.init_port_num()
        
        # Create a label for the dimensions
        self.init_dimensions()
        
        # Create a label for the framerate
        self.init_framerate()

        self.init_recording_time()

        self.init_crf_value()

        self.init_basename()

        self.init_maxdepth()

        self.init_mindepth()

        ## Add button 
        
        self.label_vid_save_dir.grid(row=self.row_num, column=0, sticky='w')
        self.btn_open_dir.grid(row=self.row_num, column=1, sticky='w')
        self.row_num += 1

        self.label_json.grid(row=self.row_num, column=0, sticky='w')
        self.btn_open_json.grid(row=self.row_num, column=1, sticky='w')
        self.row_num += 1

        self.ir_label.grid(row=self.row_num, column=0, sticky='w')
        self.ir_on.grid(row=self.row_num, column=1, sticky='w')
        self.row_num += 1

        self.color_label.grid(row=self.row_num, column=0, sticky='w')
        self.color_on.grid(row=self.row_num, column=1, sticky='w')
        self.row_num += 1

        self.align_label.grid(row=self.row_num, column=0, sticky='w')
        self.align_on.grid(row=self.row_num, column=1, sticky='w')
        self.row_num += 1

        self.btn_start.grid(row=self.row_num, column=0, sticky='w')
        self.row_num += 1


    def dir_dialog(self):
        dirname = filedialog.askdirectory(**self.dir_opt)
        if dirname:
            self.vid_save_dir = dirname
            self.label_vid_save_dir.config(text="Video save directory: " + self.vid_save_dir)
            
        else:
            print("Error: Directory must not be empty.")
    
    def json_dialog(self):
        filename = filedialog.askopenfilename(**self.json_opt)
        if filename:
            self.json_file = filename
            self.label_json.config(text="JSON file: " + self.json_file)
            if not os.path.exists(self.json_file):
                print("Error: File does not exist.")
                self.json_file = None
                return
        else:
            print("Error: File not selected.")
            self.json_file = None


    def start_recording(self):
        try:

            self.total_time_sec = int(self.hour_entry.get()) * 3600 + int(self.minute_entry.get()) * 60 + int(self.second_entry.get())
        except Exception as e:
            print(e)
            tkinter.messagebox.showerror(title="Error", message="Invalid time entered. Please enter time in seconds.")
            return
        self.time_start = time.time()
        if not self.json_file:
            tkinter.messagebox.showerror(title="Error", message="No JSON file selected.")
            return
        if not self.vid_save_dir:
            tkinter.messagebox.showerror(title="Error", message="No video save directory selected.")
            return
        if not self.total_time_sec > 0:
            tkinter.messagebox.showerror(title="Error", message="Invalid time entered. Please enter time in seconds.")
            return
        #self.clock_thread.start()


        
        print("Started encoding and recording")
        ## TODO call server instance here
        self.run_server()

        print("Ended recording")

        tkinter.messagebox.showinfo(title="Success", message="Recording complete.")
        #self.root.destroy()

    def run_server(self):

        args_use = {}
        args_use['port'] = int(self.port_entry.get())
        args_use['loglevel'] = 'info'
        dimen = self.dimensions_entry.get().split('x')
        args_use['dimensions'] = dimen[0] + 'x' + dimen[1]

        args_use['fps'] = int(self.framerate_entry.get())
        
        secon = self.second_entry.get()
        minu = self.minute_entry.get()
        hour = self.hour_entry.get()
        args_use['time_run'] = int(hour) * 3600 + int(minu) * 60 + int(secon) 
        args_use['json'] = self.json_file
        args_use['crf'] = int(self.crf_spinbox.get())
        args_use['basename'] = self.base_name_entry.get()
        args_use['max_depth'] = int(self.max_depth_spinbox.get())
        args_use['min_depth'] = int(self.min_depth_spinbox.get())
        args_use['depth_unit'] = 1000
        args_use['num_clients'] = len(self.ip_lst)
        args_use['dir'] = self.vid_save_dir
        args_use['ir'] = 1
        args_use['color'] = 0
        args_use['aligned_to_color'] = 0

        server = Server(self.ip_lst, **args_use)
        cmd_list = build_ffmpeg_cmd_group_list(server.ffmpeg_port_map, server.argdict['loglevel'], server.server_ip, server.argdict['dir'], bool(server.argdict['color'] != 0), bool(server.argdict['ir'] !=0))
        p1 = multiprocessing.Process(target=run_processes_parallel, args=(cmd_list,))
        p1.start()
        server.listen()
        # if p1.is_alive():
        #     p1.terminate()
        #     p1.join()
        try:
            p1.join()
        except Exception as e:
            p1.terminate()
            p1.join()
            print("Error here: {}".format(e))
            #pass
        finally:
            os.system("stty echo")


    def init_recording_time(self):
        # Create a label for the recording time
        self.hours = -1
        self.minutes = -1
        self.seconds = -1
        vcmd = (self.register(self.validate_int), '%d', '%i', '%P', '%s', '%S', '%v', '%V', '%W')
        sub_grid = ttk.Frame(self)
        self.time_record_label = ttk.Label(sub_grid, text="Time to record:")
        
        self.hour_entry = ttk.Spinbox(sub_grid, from_=0, to=200, width=3, validate='key', validatecommand=vcmd)
        self.hour_entry.delete(0, 'end')
        self.hour_entry.insert(0, "0")
        self.minute_entry = ttk.Spinbox(sub_grid, from_=0, to=59, width=2, validate='key', validatecommand=vcmd)
        self.minute_entry.delete(0, 'end')
        self.minute_entry.insert(0, "0")
        self.second_entry = ttk.Spinbox(sub_grid, from_=0, to=59, width=2, increment=1, validate='key', validatecommand=vcmd)
        self.second_entry.delete(0, 'end')
        self.second_entry.insert(30, "30")

        # Sub grid for the time
        # New frame


        self.time_record_label.grid(row=0, column=0, padx=5, pady=5, sticky='w')
        # Sub grid for the time
        self.hour_entry.grid(row=0, column=1, padx=5, pady=5)
        self.minute_entry.grid(row=0, column=2, padx=5, pady=5)
        self.second_entry.grid(row=0, column=3, padx=5, pady=5)


        sub_grid.grid(row=self.row_num, column=0, padx=5, pady=5, sticky='w')
        self.row_num += 1
        


    def init_port_num(self):

        sub_grid = ttk.Frame(self)
        self.port_label = ttk.Label(sub_grid, text="Port number: ")
        # Create a text box for the port number
        self.port_text = tkinter.IntVar()
        self.port_text.set(5000)
        self.port_entry = ttk.Spinbox(sub_grid, from_=5000, increment=1, to=5000+600, width=5, textvariable=self.port_text, command=self.__check_port)
        self.port_label.grid(row=0, column=0, padx=5, pady=5, sticky='w')
        self.port_entry.grid(row=0, column=1, padx=5, pady=5, sticky='w')

        sub_grid.grid(row=self.row_num, column=0, padx=5, pady=5, sticky='w')
        self.row_num += 1

    def init_dimensions(self):
        sub_grid = ttk.Frame(self)
        self.dimensions_label = ttk.Label(sub_grid, text="Video dimensions: ")
        # Create a text box for the video dimensions
        self.dimensions_text = tkinter.StringVar()
        self.dimensions_text.set("1280x720")
        self.dimensions_entry = ttk.Entry(sub_grid, textvariable=self.dimensions_text, width=10)
        self.dimensions_label.grid(row=0, column=0, padx=5, pady=5, sticky='w')
        self.dimensions_entry.grid(row=0, column=1, padx=5, pady=5, sticky='w')

        sub_grid.grid(row=self.row_num, column=0, padx=5, pady=5, sticky='w')
        self.row_num += 1

    def init_framerate(self):
        sub_grid = ttk.Frame(self)
        self.framerate_label = ttk.Label(sub_grid, text="Framerate: ")
        # Create a text box for the framerate
        self.framerate_text = tkinter.IntVar()
        self.framerate_text.set(30)
        self.framerate_entry = ttk.Spinbox(sub_grid, from_=1, increment=1, to=60, textvariable=self.framerate_text, command=self.__check_framerate)

        self.framerate_label.grid(row=0, column=0, padx=5, pady=5, sticky='w')
        self.framerate_entry.grid(row=0, column=1, padx=5, pady=5, sticky='w')
        
        sub_grid.grid(row=self.row_num, column=0, padx=5, pady=5, sticky='w')
        self.row_num += 1

    def init_crf_value(self):
        # Create a label for the CRF value

        sub_grid = ttk.Frame(self)
        self.crf_label = ttk.Label(sub_grid, text="CRF value: ")
        # Create a text box for the CRF value
        self.crf_text = tkinter.IntVar()
        self.crf_text.set(22)
        self.crf_spinbox = ttk.Spinbox(sub_grid, from_=0, to=51, textvariable=self.crf_text)

        self.crf_label.grid(row=0, column=0, padx=5, pady=5, sticky='w')
        self.crf_spinbox.grid(row=0, column=1, padx=5, pady=5, sticky='w')

        sub_grid.grid(row=self.row_num, column=0, padx=5, pady=5, sticky='w')
        self.row_num += 1

    def init_basename(self):
        # Create a label for the base name of the video files
        sub_grid = ttk.Frame(self)
        self.base_name_label = ttk.Label(sub_grid, text="Base name of video files: ")
        # Create a text box for the base name of the video files
        self.base_name_text = tkinter.StringVar()
        self.base_name_text.set("test_")
        self.base_name_entry = ttk.Entry(sub_grid, textvariable=self.base_name_text)

        self.base_name_label.grid(row=0, column=0, padx=5, pady=5, sticky='w')
        self.base_name_entry.grid(row=0, column=1, padx=5, pady=5, sticky='w')

        sub_grid.grid(row=self.row_num, column=0, padx=5, pady=5, sticky='w')
        self.row_num += 1

    def init_maxdepth(self):
        # Create a label for the max depth
        sub_grid = ttk.Frame(self)
        self.max_depth_label = ttk.Label(sub_grid, text="Max depth: ")
        # Create a text box for the max depth
        self.max_depth_text = tkinter.IntVar()
        self.max_depth_text.set((2**16)-1)
        self.max_depth_spinbox = ttk.Spinbox(sub_grid, from_=1, increment=1, to=(2**16)-1, textvariable=self.max_depth_text)

        self.max_depth_label.grid(row=0, column=0, padx=5, pady=5, sticky='w')
        self.max_depth_spinbox.grid(row=0, column=1, padx=5, pady=5, sticky='w')

        sub_grid.grid(row=self.row_num, column=0, padx=5, pady=5, sticky='w')
        self.row_num += 1

    def init_mindepth(self):
        # Create a label for the min depth
        sub_grid = ttk.Frame(self)
        self.min_depth_label = ttk.Label(sub_grid, text="Min depth: ")
        # Create a text box for the min depth
        self.min_depth_text = tkinter.IntVar()
        self.min_depth_text.set(0)
        self.min_depth_spinbox = ttk.Spinbox(sub_grid, from_=1, increment=1, to=(2**16)-1, textvariable=self.min_depth_text)

        self.min_depth_label.grid(row=0, column=0, padx=5, pady=5, sticky='w')
        self.min_depth_spinbox.grid(row=0, column=1, padx=5, pady=5, sticky='w')

        sub_grid.grid(row=self.row_num, column=0, padx=5, pady=5, sticky='w')
        self.row_num += 1

    def __check_port(self):
        port = self.port_text.get()
        if port < 1024 or port > 65535:
            messagebox.showerror("Error", "Port number must be between 1024 and 65535")
            self.port_text.set(5000)

    def __check_dimensions(self):
        dimensions = self.dimensions_text.get()
        if not re.match(r"^[0-9]{3,4}x[0-9]{3,4}$", dimensions):
            messagebox.showerror("Error", "Video dimensions must be in the format WIDTHxHEIGHT")
            self.dimensions_text.set("1280x720")
            
    def __check_framerate(self):
        framerate = self.framerate_text.get()
        if framerate < 1 or framerate > 60:
            messagebox.showerror("Error", "Framerate must be between 1 and 60")
            self.framerate_text.set(30)

    def validate_int(self, action, index, value_if_allowed, prior_value, text, validation_type, trigger_type, widget_name):
        if value_if_allowed == "":
            # If the entry is empty,change it to 0
            if self.hour_entry.get() == "":
                self.hour_entry.insert(0, "0")
            if self.minute_entry.get() == "":
                self.minute_entry.insert(0, "0")
            if self.second_entry.get() == "":
                self.second_entry.insert(0, "0")
            return True
        if value_if_allowed:
            try:
                int(value_if_allowed)
                return True
            except ValueError:
                return False
        # elif on backspace, delete, or other key
        else:
            return False

    def run(self):
        self.root.mainloop()


def funct_to_call():
    print("hjh")

def run_gui():
    print("Getting IP addresses...")
    ip_lst = map_network()
    print("Done getting IP addresses")
    print("Starting Graphical User Interface...")
    parent = tkinter.Tk()

    app = ServerGUI(parent, ip_lst)
    app.run()

if __name__ == "__main__":
    run_gui()