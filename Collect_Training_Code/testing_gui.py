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
from extract_ffmpeg_frames import parallel_call
import abc

# class TkFileDialogExample(tk.Frame):

#   def __init__(self, root):

#     tk.Frame.__init__(self, root)

#     # options for buttons
#     button_opt = {'fill': tk.BOTH, 'padx': 5, 'pady': 5}

#     # define buttons
#     #tk.Button(self, text='askopenfile', command=self.askopenfile).pack(**button_opt)
#     tk.Button(self, text='Depth video Recording ', command=self.askopenfilename).pack(**button_opt)
#     #tk.Button(self, text='asksaveasfile', command=self.asksaveasfile).pack(**button_opt)
#     #tk.Button(self, text='asksaveasfilename', command=self.asksaveasfilename).pack(**button_opt)
#     tk.Button(self, text='askdirectory', command=self.askdirectory).pack(**button_opt)

#     # define options for opening or saving a file
#     self.file_opt = options = {}
#     options['defaultextension'] = '.txt'
#     options['filetypes'] = [('all files', '.*'), ('text files', '.txt')]
#     options['initialdir'] = 'C:\\'
#     options['initialfile'] = 'myfile.txt'
#     options['parent'] = root
#     options['title'] = 'This is a title'


#     self.dir_opt = options = {}
#     options['initialdir'] = 'C:\\'
#     options['mustexist'] = False
#     options['parent'] = root
#     options['title'] = 'This is a title'


#   def askopenfilename(self):

#     """Returns an opened file in read mode.
#     This time the dialog just returns a filename and the file is opened by your own code.
#     """

#     # get filename
#     filename = filedialog.askopenfilename(**self.file_opt)

#     return filename

# #   def asksaveasfile(self):

# #     """Returns an opened file in write mode."""

# #     return filedialog.asksaveasfile(mode='w', **self.file_opt)

# #   def asksaveasfilename(self):

# #     """Returns an opened file in write mode.
# #     This time the dialog just returns a filename and the file is opened by your own code.
# #     """

# #     # get filename
# #     filename = filedialog.asksaveasfilename(**self.file_opt)

# #     # open file on your own
# #     if filename:
# #       return open(filename, 'w')

#   def askdirectory(self):

#     """Returns a selected directoryname."""
#     dirname = filedialog.askdirectory(**self.dir_opt)
#     return dirname

# if __name__=='__main__':
#   root = tk.Tk()
#   TkFileDialogExample(root).pack()
#   root.mainloop()

class Menubar(ttk.Frame):
    """Builds a menu bar for the top of the main window"""
    def __init__(self, parent, *args, **kwargs):
        ''' Constructor'''
        ttk.Frame.__init__(self, parent, *args, **kwargs)
        self.root = parent
        self.init_menubar()

    def on_exit(self):
        '''Exits program'''
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

# class Window(ttk.Frame):
#     """Abstract base class for a popup window"""
#     __metaclass__ = abc.ABCMeta
#     def __init__(self, parent):
#         ''' Constructor '''
#         ttk.Frame.__init__(self, parent)
#         self.parent = parent
#         self.parent.resizable(width=False, height=False) # Disallows window resizing
#         self.validate_notempty = (self.register(self.notEmpty), '%P') # Creates Tcl wrapper for python function. %P = new contents of field after the edit.
#         self.init_gui()

#     @abc.abstractmethod # Must be overwriten by subclasses
#     def init_gui(self):
#         '''Initiates GUI of any popup window'''
#         pass

#     @abc.abstractmethod
#     def do_something(self):
#         '''Does something that all popup windows need to do'''
#         pass

#     def notEmpty(self, P):
#         '''Validates Entry fields to ensure they aren't empty'''
#         if P.strip():
#             valid = True
#         else:
#             print("Error: Field must not be empty.") # Prints to console
#             valid = False
#         return valid

#     def close_win(self):
#         '''Closes window'''
#         self.parent.destroy()

# class SomethingWindow(Window):
#     """ New popup window """

#     def init_gui(self):
#         self.parent.title("New Window")
#         self.parent.columnconfigure(0, weight=1)
#         self.parent.rowconfigure(3, weight=1)

#         # Create Widgets

#         self.label_title = ttk.Label(self.parent, text="This sure is a new window!")
#         self.contentframe = ttk.Frame(self.parent, relief="sunken")

#         self.label_test = ttk.Label(self.contentframe, text='Enter some text:')
#         self.input_test = ttk.Entry(self.contentframe, width=30, validate='focusout', validatecommand=(self.validate_notempty))

#         self.btn_do = ttk.Button(self.parent, text='Action', command=self.do_something)
#         self.btn_cancel = ttk.Button(self.parent, text='Cancel', command=self.close_win)

#         # Layout
#         self.label_title.grid(row=0, column=0, columnspan=2, sticky='nsew')
#         self.contentframe.grid(row=1, column=0, columnspan=2, sticky='nsew')

#         self.label_test.grid(row=0, column=0)
#         self.input_test.grid(row=0, column=1, sticky='w')

#         self.btn_do.grid(row=2, column=0, sticky='e')
#         self.btn_cancel.grid(row=2, column=1, sticky='e')

#         # Padding
#         for child in self.parent.winfo_children():
#             child.grid_configure(padx=10, pady=5)
#         for child in self.contentframe.winfo_children():
#             child.grid_configure(padx=5, pady=2)

#     def do_something(self):
#         '''Does something'''
#         text = self.input_test.get().strip()
#         if text:
#             # Do things with text
#             self.close_win()
#         else:
#             print("Error: But for real though, field must not be empty.")

class GUI(ttk.Frame):
    """Main GUI class"""
    def __init__(self, parent, *args, **kwargs):
        ttk.Frame.__init__(self, parent, *args, **kwargs)
        self.root = parent
        self.init_gui()

        

    # def openwindow(self):
    #     self.new_win = tkinter.Toplevel(self.root) # Set parent
    #     SomethingWindow(self.new_win)




    def init_gui(self):
        self.root.title('Test GUI')
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
        else:
            print("Error: File not selected.")

    def rgb_file_dialog(self):
        filename = filedialog.askopenfilename(**self.file_opt_rgb)
        if filename:
            self.rgb_file = filename
            self.label_rgb.config(text="Color file: " + self.rgb_file)
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