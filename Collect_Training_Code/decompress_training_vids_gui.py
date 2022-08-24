import tkinter
# Lots of tutorials have from tkinter import *, but that is pretty much always a bad idea
from tkinter import ttk
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
        message_out += "First, select the depth video files you want to decompress (usually starting with depth_vid<DATE>.mkv)\nThen select the corresponding color video files (usually starting with color_vid<DATE>.mp4).\n"
        message_out += "Then, select the directory where you want the output to be saved.\nThen select the number of frames to process, in this case we are recording at 30 fps, so 30 frames is equivalent to 1 second of video.\n"
        message_out += "The output will be a collection of .tif files in the directory you selected, each depth frame being numbered with its corresponding rgb frame.\n"
        # Change font of message box
        #tkinter.Tk().option_add('*Dialog.msg.font', 'Helvetica 12')
        tkinter.messagebox.showinfo(title="Decompression Program", message=message_out)
        #tkinter.messagebox.showinfo(title="Decompression Program", message=message_out)

    def display_about(self):
        '''Displays info about program'''
        message_out = "Directions for use of this program\n"
        message_out += "First, select the depth video files you want to decompress (usually starting with depth_vid<DATE>.mkv)\nThen select the corresponding color video files (usually starting with color_vid<DATE>.mp4).\n"
        message_out += "Then, select the directory where you want the output to be saved.\nThen select the number of frames to process, in this case we are recording at 30 fps, so 30 frames is equivalent to 1 second of video.\n"
        message_out += "The output will be a collection of .tif files in the directory you selected, each depth frame being numbered with its corresponding rgb frame.\n"
        tkinter.messagebox.showinfo(title="Decompression Program", message=message_out)

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


    def init_gui(self):
        self.root.title('Decompress Training Videos for Depth Camera')
        self.root.geometry("600x400")
        self.grid(column=0, row=0, sticky='nsew')
        self.grid_columnconfigure(0, weight=1) # Allows column to stretch upon resizing
        self.grid_rowconfigure(0, weight=1) # Same with row
        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_rowconfigure(0, weight=1)
        self.root.option_add('*tearOff', 'FALSE') # Disables ability to tear menu bar into own window
        self.depth_file = None
        self.rgb_file = None
        self.dir_out_path = None
        self.label_depth = ttk.Label(self, text="Depth File:")
        self.label_rgb = ttk.Label(self, text="RGB File:")
        self.label_dir = ttk.Label(self, text="Output Directory:")
        self.num_frames = -1


        self.file_opt_rgb = {}
        self.file_opt_rgb['defaultextension'] = '.mp4'
        self.file_opt_rgb['filetypes'] = [('mp4 files', '.mp4')]
        self.file_opt_rgb['parent'] = root
        self.file_opt_rgb['title'] = 'Select regular color MP4 file'

        self.file_opt_depth = {}
        self.file_opt_depth['defaultextension'] = '.mkv'
        self.file_opt_depth['filetypes'] = [('mkv files', '.mkv')]
        self.file_opt_depth['parent'] = root
        self.file_opt_depth['title'] = 'Select depth MKV file'



        self.dir_opt = {}
        #self.dir_opt['initialdir'] = '/home/'
        self.dir_opt['mustexist'] = True
        self.dir_opt['parent'] = root
        self.dir_opt['title'] = 'Choose a directory to save TIF files to.'
        # Menu Bar
        self.menubar = Menubar(self.root)
        
        # Create multiple Widgets
        # self.btn_open = ttk.Button(self, text='Open', command=self.openwindow)
        
        # Button with file dialog change text to filename
        #self.label_depth = ttk.Label(self, text='Depth File:')


        self.btn_open_depth = ttk.Button(self, text='Open .mkv depth file', command=self.depth_file_dialog)

        # Change btn_open_depth text to filename
        # if self.depth_file:
        #     self.btn_open_depth.config(text=self.depth_file)
        #self.label_rgb = ttk.Label(self, text='RGB File:')

        self.btn_open_rgb = ttk.Button(self, text='Open .mp4 color file', command=self.rgb_file_dialog)
        # if self.rgb_file:
        #     self.btn_open_rgb.config(text=self.rgb_file)
        #self.label_out = ttk.Label(self, text='Output Directory:')

        self.btn_open_dir = ttk.Button(self, text='Open Dir', command=self.dir_dialog)

        self.btn_start = ttk.Button(self, text='Start Decompressing', command=self.run_decompress)
        

        # Number of frames using spinbox
        self.label_num_frames = ttk.Label(self, text='Number of frames to decompress:')
        # Default value is 1
        self.spin_num_frames = ttk.Spinbox(self, from_=1, to=2147483647, increment=1, command=lambda: self.print_int(None))
        self.spin_num_frames.delete(0, 'end')
        self.spin_num_frames.insert(0, '1')
        # Update on enter
        self.spin_num_frames.bind('<Return>', self.print_int)




        #self.spin_num_frames = ttk.Spinbox(self, from_=1, to=100000000000, increment=1, command= self.print_int)
        # Layout using grid
        self.label_depth.grid(row=0, column=0, sticky='w')
        self.btn_open_depth.grid(row=0, column=1, sticky='w')
        self.label_rgb.grid(row=1, column=0, sticky='w')
        self.btn_open_rgb.grid(row=1, column=1, sticky='w')
        self.label_dir.grid(row=2, column=0, sticky='w')
        self.btn_open_dir.grid(row=2, column=1, sticky='w')
        self.label_num_frames.grid(row=3, column=0, sticky='w')
        self.spin_num_frames.grid(row=3, column=1, sticky='w')
        self.btn_start.grid(row=4, column=0, sticky='w')

        
        # Padding
        for child in self.winfo_children():
            child.grid_configure(padx=10, pady=5)

        # Raise error if self.depth_file is None or self.rgb_file is None or self.dir_out_path is None or self.num_frames is None
        # if self.depth_file is None or self.rgb_file is None or self.dir_out_path is None or self.num_frames < 0:
        #     print("Error: One or more files not selected.")


    def depth_file_dialog(self):
        filename = filedialog.askopenfilename(**self.file_opt_depth)
        if filename:
            self.depth_file = filename
            self.label_depth.config(text="Depth file: " + self.depth_file)
            if not os.path.exists(self.depth_file):
                print("Error: File does not exist.")
                self.rgb_file = None
                return
        else:
            print("Error: File not selected.")

    def rgb_file_dialog(self):
        filename = filedialog.askopenfilename(**self.file_opt_rgb)
        if filename:
            self.rgb_file = filename
            self.label_rgb.config(text="Color file: " + self.rgb_file)
            if not os.path.exists(self.rgb_file):
                print("Error: File does not exist.")
                self.rgb_file = None
                return
        else:
            print("Error: File not selected.")

    def dir_dialog(self):
        dirname = filedialog.askdirectory(**self.dir_opt)
        if dirname:
            self.dir_out_path = dirname
            self.label_dir.config(text="Output directory: " + self.dir_out_path)

        else:
            print("Error: Directory must not be empty.")

    def print_int(self, event):
        num_frames = self.spin_num_frames.get()
        self.num_frames = int(num_frames)        
        print(num_frames)

    def run_decompress(self):
        self.num_frames = int(self.spin_num_frames.get())
        if self.depth_file and self.rgb_file and self.dir_out_path and self.num_frames > 0:
            # depth file ends with .mkv
            # rgb file ends with .mp4
            # dir out is valid
            try:
                parallel_call(self.depth_file, self.rgb_file, self.dir_out_path, self.num_frames)
            except Exception as e:
                print(e)
                print("Error: Something went wrong.")
                self.root.destroy()
                return
            
            # Show success message
            tkinter.messagebox.showinfo(title="Decompression Dialogue", message="Decompression complete!")
            self.root.destroy()
            print("Done!")
        else:
            tkinter.messagebox.showinfo(title="Decompression Dialogue", message="Error: One or more files not selected.")
            print("Error: One or more fields not filled.")

           
    def run(self):
        self.root.mainloop()
        
if __name__ == '__main__':
    root = tkinter.Tk()
    gui = GUI(root)
    gui.run()