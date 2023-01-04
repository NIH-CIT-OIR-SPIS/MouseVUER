#!/usr/bin/python3
import tkinter as tk
import platform
from tkinter import filedialog
from tkinter import ttk
import re

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
    def __init__(self, parent, ip_table_dict, *args, **kwargs):
        ''' Constructor'''
        ttk.Frame.__init__(self, parent, *args, **kwargs)
        self.root = parent
        self.ip_table_dict = ip_table_dict
        self.init_server_gui()

    def init_server_gui(self):
        # Set the title of the main window
        self.root.title("Depth Realtime recording and encoding program")
        # Set the size of the main window
        self.root.geometry("800x600")
        # Create a menu bar
        self.menubar = Menubar(self.root)

        # Create a frame for the widgets
        self.widgets_frame = ttk.Frame(self.root)
        self.widgets_frame.pack(fill=tk.BOTH, expand=True)


        # Create a label for the port number
        self.port_label = ttk.Label(self.widgets_frame, text="Port number: ")
        self.port_label.grid(row=0, column=0, padx=5, pady=5)
        
        # Create a text box for the port number
        self.port_text = tk.IntVar()
        self.port_text.set(5000)
        self.port_entry = ttk.Entry(self.widgets_frame, textvariable=self.port_text, command=self.__check_port)
        self.port_entry.grid(row=0, column=1, padx=5, pady=5)

        # Create a label for the video dimensions
        self.dimensions_label = ttk.Label(self.widgets_frame, text="Video dimensions: ")
        self.dimensions_label.grid(row=1, column=0, padx=5, pady=5)
        # Create a text box for the video dimensions
        self.dimensions_text = tk.StringVar()
        self.dimensions_text.set("1280x720")
        self.dimensions_entry = ttk.Entry(self.widgets_frame, textvariable=self.dimensions_text, command=self.__check_dimensions)
        self.dimensions_entry.grid(row=1, column=1, padx=5, pady=5)
        
        # Create a label for the framerate
        self.framerate_label = ttk.Label(self.widgets_frame, text="Framerate: ")
        self.framerate_label.grid(row=2, column=0, padx=5, pady=5)
        # Create a text box for the framerate
        self.framerate_text = tk.StringVar()
        self.framerate_text.set("30")
        self.framerate_entry = ttk.Entry(self.widgets_frame, textvariable=self.framerate_text)
        self.framerate_entry.grid(row=2, column=1, padx=5, pady=5)

        # Create a label for the recording time
        self.recording_time_label = ttk.Label(self.widgets_frame, text="Recording time: ")
        self.recording_time_label.grid(row=3, column=0, padx=5, pady=5)
        # Create a text box for the recording time
        self.recording_time_text = tk.StringVar()
        self.recording_time_text.set("00:00:30")
        self.recording_time_entry = ttk.Entry(self.widgets_frame, textvariable=self.recording_time_text)
        self.recording_time_entry.grid(row=3, column=1, padx=5, pady=5)

        # Create a label for the CRF value
        self.crf_label = ttk.Label(self.widgets_frame, text="CRF value: ")
        self.crf_label.grid(row=4, column=0, padx=5, pady=5)
        # Create a text box for the CRF value
        self.crf_text = tk.StringVar()
        self.crf_text.set("22")
        self.crf_entry = ttk.Entry(self.widgets_frame, textvariable=self.crf_text)
        self.crf_entry.grid(row=4, column=1, padx=5, pady=5)

        # Create a label for the base name of the video files
        self.base_name_label = ttk.Label(self.widgets_frame, text="Base name of video files: ")
        self.base_name_label.grid(row=5, column=0, padx=5, pady=5)
        # Create a text box for the base name of the video files
        self.base_name_text = tk.StringVar()
        self.base_name_text.set("test_")
        self.base_name_entry = ttk.Entry(self.widgets_frame, textvariable=self.base_name_text)
        self.base_name_entry.grid(row=5, column=1, padx=5, pady=5)

        # Create a label for the max depth of the depth camera
        self.max_depth_label = ttk.Label(self.widgets_frame, text="Max depth of depth camera: ")
        self.max_depth_label.grid(row=6, column=0, padx=5, pady=5)
        # Create a text box for the max depth of the depth camera
        self.max_depth_text = tk.StringVar()
        self.max_depth_text.set("1000")
        self.max_depth_entry = ttk.Entry(self.widgets_frame, textvariable=self.max_depth_text)
        self.max_depth_entry.grid(row=6, column=1, padx=5, pady=5)

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
    
