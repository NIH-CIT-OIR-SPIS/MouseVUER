import tkinter
# Lots of tutorials have from tkinter import *, but that is pretty much always a bad idea
from tkinter import ttk
import re
import os
import argparse
import numpy as np
import subprocess as sp
import multiprocessing
import time
import shlex
import signal 
import sys
import platform
from tkinter import filedialog
from extract_ffmpeg_frames_helper import parallel_call



def make_directory(path: str):
    # Makes a directory with all permissions
    if not os.path.exists(path):
        os.makedirs(path, mode=0o777)
    else:
        print("Directory already exists: {}".format(path))



# This is a GUI for decompressing videos using ffmpeg
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
        # Change font of message box
        #tkinter.Tk().option_add('*Dialog.msg.font', 'Helvetica 12')
        tkinter.messagebox.showinfo(title="Recording Program", message=message_out)
        #tkinter.messagebox.showinfo(title="Decompression Program", message=message_out)

    def display_about(self):
        '''Displays info about program'''
        pass

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


class GUI(ttk.Frame):
    """Main GUI class"""
    def __init__(self, parent, *args, **kwargs):
        ttk.Frame.__init__(self, parent, *args, **kwargs)
        self.root = parent
        self.init_gui()


    def run_make_file_check(self):
        os.system("make -j4")
        if not os.path.exists("bin/collect_raw") or not os.path.exists("bin/util_dec"):
            tkinter.messagebox.showerror(title="Error", message="Make failed. Check console output for errors.")
            self.root.destroy()

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


    def start_recording(self):
        pass

    def run(self):
        self.root.mainloop()

    def init_gui(self):
        self.root.title('Collect aligned training frames from depth and color videos')
        self.root.geometry("600x400")
        self.grid(column=0, row=0, sticky='nsew')
        self.grid_columnconfigure(0, weight=1) # Allows column to stretch upon resizing
        self.grid_rowconfigure(0, weight=1) # Same with row
        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_rowconfigure(0, weight=1)
        self.root.option_add('*tearOff', 'FALSE') # Disables ability to tear menu bar into own window
        if not os.path.exists("bin/collect_raw") or not os.path.exists("bin/util_dec"):
            self.run_make_file_check()



        self.menubar = Menubar(self.root)

        self.dir_opt = {}
        #self.dir_opt['initialdir'] = '/home/'
        self.dir_opt['mustexist'] = True
        self.dir_opt['parent'] = root
        self.dir_opt['title'] = 'Choose a directory to save aligned color MP4, and depth MKV files.'

        self.json_opt = {}
        self.json_opt['defaultextension'] = '.json'
        self.json_opt['parent'] = root
        self.json_opt['title'] = 'Choose a JSON file to use for alignment.'
        self.json_opt['filetypes'] = [('JSON files', '.json')]
        



        self.vid_save_dir = None
        self.json_file = None

        self.hours = tkinter.IntVar(0)
        self.minutes = tkinter.IntVar(0)
        self.seconds = tkinter.IntVar(0)

        self.time_record_label = ttk.Label(self, text="Time to record:")
        self.hour_entry = ttk.Entry(self, textvariable=self.hours)
        self.minute_entry = ttk.Entry(self, textvariable=self.minutes)
        self.second_entry = ttk.Entry(self, textvariable=self.seconds)


        self.label_vid_save_dir = ttk.Label(self, text="Video save directory: ")
        self.btn_open_dir = ttk.Button(self, text='Save Directory', command=self.dir_dialog)

        self.label_json = ttk.Label(self, text="JSON file: ")
        self.btn_open_json = ttk.Button(self, text='Open JSON', command=self.json_dialog)
        
        self.btn_start = ttk.Button(self, text='Start', command=self.start_recording)
        
        # Padding
        for child in self.winfo_children():
            child.grid_configure(padx=10, pady=5)

        # Raise error if self.depth_file is None or self.rgb_file is None or self.dir_out_path is None or self.num_frames is None
        # if self.depth_file is None or self.rgb_file is None or self.dir_out_path is None or self.num_frames < 0:
        #     print("Error: One or more files not selected.")

    

        
if __name__ == '__main__':
    root = tkinter.Tk()
    gui = GUI(root)
    gui.run()