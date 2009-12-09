﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;
using System.Configuration;

namespace D2BSLoader
{
	public partial class Main : Form
	{
		private delegate void StatusCallback(string status, System.Drawing.Color color);
		private delegate void LoadAction(int pid);

		private static string D2Path, D2Exe, D2Args, D2BSDLL;
		private BindingList<ProcessWrapper> processes = new BindingList<ProcessWrapper>();

		public bool Autoclosed { get; set; }

		public Main(string[] args)
		{
			// process command line args
			Dictionary<string, LoadAction> actions = new Dictionary<string, LoadAction>();
			actions.Add("inject", Inject);
			actions.Add("kill", Kill);
			actions.Add("start", Start);
			actions.Add("save", Save);

			string action = String.Empty,
				   path = String.Empty,
				   exe = String.Empty,
				   param = String.Empty,
				   dll = "cGuard.dll";

			int pid = -1;

			for(int i = 0; i < args.Length; i++)
			{
				switch(args[i])
				{
					case "--pid": pid = Convert.ToInt32(args[i+1]); i++; break;
					case "--dll": dll = args[i+1]; i++; break;
					case "--path": path = args[i+1]; i++; break;
					case "--exe": exe = args[i+1]; i++; break;
					case "--params":
						// treat the rest of the command line as if it were params directly to d2
						string[] args2 = new string[args.Length-i-1];
						Array.Copy(args, i+1, args2, 0, args.Length-i-1);
						param = " " + String.Join(" ", args2);
						i = args.Length;
						break;
					default: action = args[i].Substring(2); break;
				}
			}

			// copy over the specified path, exe, and dll if necessary
			if(!String.IsNullOrEmpty(path))
				D2Path = path;
			if(!String.IsNullOrEmpty(exe))
				D2Exe = exe;
			if(!String.IsNullOrEmpty(dll))
				D2BSDLL = dll;

			// merge the specified args with the official args
			D2Args = String.Join(" ", new string[] { D2Args, param });

			if((String.IsNullOrEmpty(action)) ||
			   (action == "start" && (String.IsNullOrEmpty(D2Path) || String.IsNullOrEmpty(D2Exe))))
			{
				// if the path or exe is empty, load settings
				if(String.IsNullOrEmpty(D2Path) || String.IsNullOrEmpty(D2Exe))
					ReloadSettings();
				// if the path is still empty, open the settings dialog
				if(String.IsNullOrEmpty(D2Path))
				{
					Options_Click(null, null);
					ReloadSettings();
				}
			}

			if(!File.Exists(D2Path + Path.DirectorySeparatorChar + D2Exe) ||
			   !File.Exists(Application.StartupPath + Path.DirectorySeparatorChar + D2BSDLL))
			{
				MessageBox.Show("Diablo II Executable or D2BS not found.", "D2BS");
			}

			if(!String.IsNullOrEmpty(action))
			{
				Autoclosed = true;
				actions[action](pid);
				Close();
				return;
			}

			InitializeComponent();

			processes.RaiseListChangedEvents = true;
			Processes.DataSource = processes;
			Processes.DisplayMember = "ProcessName";
			System.Threading.Thread t = new System.Threading.Thread(ListUpdateThread);

			Shown += delegate { Process.EnterDebugMode(); t.Start(); };
			FormClosing += delegate { t.Abort(); Process.LeaveDebugMode(); };
		}

		private void ListUpdateThread()
		{
			while(true)
			{
				foreach(ProcessWrapper p in processes)
					p.Process.Refresh();

				processes.RemoveAll(x => x.Process.HasExited);

				foreach(Process p in Process.GetProcesses())
				{
					if(processes.Exists(x => p.Id == x.Process.Id))
						continue;

					string moduleName = "";
					try {
						moduleName = Path.GetFileName(p.MainModule.FileName).ToLowerInvariant();
					} catch { }
					string classname = GetLCClassName(p);
					if(!String.IsNullOrEmpty(classname) && classname == "diablo ii" &&
						(moduleName == "game.exe" || moduleName.Contains("d2loader") ||
						 moduleName.Contains("d2launcher")))
					{
						ProcessWrapper pw = new ProcessWrapper(p);
						processes.Add(pw);
						if(GetAutoload())
						{
							p.WaitForInputIdle();
							Attach(pw);
						}
					}
				}
				Processes.Invoke((MethodInvoker)delegate {
					((CurrencyManager)Processes.BindingContext[Processes.DataSource]).Refresh();
				});
				System.Threading.Thread.Sleep(400);
			}
		}

		public static void SaveSettings(string path, string exe, string args, string dll)
		{
			Configuration config = ConfigurationManager.OpenExeConfiguration(ConfigurationUserLevel.None);
			config.AppSettings.Settings.Remove("D2Path");
			config.AppSettings.Settings.Remove("D2Exe");
			config.AppSettings.Settings.Remove("D2Args");
			config.AppSettings.Settings.Remove("D2BSDLL");
			config.AppSettings.Settings.Add("D2Path", path);
			config.AppSettings.Settings.Add("D2Exe", exe);
			config.AppSettings.Settings.Add("D2Args", args);
			config.AppSettings.Settings.Add("D2BSDLL", dll);
			config.Save(ConfigurationSaveMode.Full);
		}

		private static void ReloadSettings()
		{
			ConfigurationManager.RefreshSection("appSettings");
			D2Path = ConfigurationManager.AppSettings["D2Path"];
			D2Exe = ConfigurationManager.AppSettings["D2Exe"];
			D2Args = ConfigurationManager.AppSettings["D2Args"];
			D2BSDLL = ConfigurationManager.AppSettings["D2BSDLL"];
		}

		private bool GetAutoload()
		{
			bool autoload = false;
			if(Autoload.InvokeRequired)
				Autoload.Invoke((MethodInvoker)delegate { autoload = Autoload.Checked; });
			else
				autoload = Autoload.Checked;
			return autoload;
		}

		private void SetStatus(string status, Color color)
		{
			// ignore the call if we haven't init'd the window yet
			if(Status == null)
				return;

			if(Status.InvokeRequired)
			{
				StatusCallback cb = SetStatus;
				Status.Invoke(cb, status, color);
				return;
			}
			Status.ForeColor = color;
			Status.Text = status;
		}

		private static string GetLCClassName(Process p) { return PInvoke.User32.GetClassNameFromProcess(p).ToLowerInvariant(); }
		private static Process GetProcessById(int pid)
		{
			try { return Process.GetProcessById(pid); }
			catch(ArgumentException) { return null; }
		}

		private void Start(int pid)
		{
			int id = Start();
			Inject(id);
			if(Autoclosed)
				Console.WriteLine(pid);
		}
		public static void Inject(int pid)
		{
			Process p = GetProcessById(pid);
			if(p != null && GetLCClassName(p) == "diablo ii")
				Attach(p);
		}
		public static void Kill(int pid)
		{
			Process p = GetProcessById(pid);
			if(p != null && GetLCClassName(p) == "diablo ii")
				p.Kill();
		}
		private void Save(int pid)
		{
			SaveSettings(D2Path, D2Exe, D2Args, D2BSDLL);
		}

		private static bool Attach(Process p)
		{
			string path = Application.StartupPath + Path.DirectorySeparatorChar;
			return  File.Exists(path + "libnspr4.dll") &&
					File.Exists(path + "js32.dll") &&
					File.Exists(path + D2BSDLL) &&
					PInvoke.Kernel32.LoadRemoteLibrary(p, path + "libnspr4.dll") &&
					PInvoke.Kernel32.LoadRemoteLibrary(p, path + "js32.dll") &&
					PInvoke.Kernel32.LoadRemoteLibrary(p, path + D2BSDLL);
		}

		private void Attach(ProcessWrapper pw)
		{
			if(pw.Loaded)
				SetStatus("Already loaded!", Color.Blue);
			else if(Attach(pw.Process))
			{
				SetStatus("Success!", Color.Green);
				pw.Loaded = true;
			}
			else
				SetStatus("Failed!", Color.Red);
		}

		public static int Start(string path, string exe, string param, string dll)
		{
			D2Path = path;
			D2Exe = exe;
			D2Args = param;
			D2BSDLL = dll;
			return Start();
		}

		public static int Start(params string[] args)
		{
			ReloadSettings();
			D2Args = String.Join(" ", args);
			return Start();
		}

		private static int Start()
		{
			if(!File.Exists(D2Path + Path.DirectorySeparatorChar + D2Exe) ||
			   !File.Exists(Application.StartupPath + Path.DirectorySeparatorChar + D2BSDLL))
				return -1;

			ProcessStartInfo psi = new ProcessStartInfo(D2Path + Path.DirectorySeparatorChar + D2Exe, D2Args);
			psi.UseShellExecute = false;
			psi.WorkingDirectory = D2Path;
			Process p = Process.Start(psi);
			return p.Id;
		}

		private void Load_Click(object sender, EventArgs e)
		{
			if(Processes.SelectedIndex == -1)
			{
				SetStatus("No process selected!", Color.Blue);
				return;
			}
			Attach(Processes.SelectedItem as ProcessWrapper);
		}

		private void StartNew_Click(object sender, EventArgs e)
		{
			Start();
		}

		private void Options_Click(object sender, EventArgs e)
		{
			Options o = new Options(D2Path, D2Exe, D2Args, D2BSDLL);
			o.ShowDialog();
			ReloadSettings();
		}
	}

	internal class ProcessWrapper
	{
		public bool Loaded { get; set; }
		public Process Process { get; internal set; }
		public string ProcessName {
			get {
				if(Process.HasExited)
					return "Exited...";
				return Process.MainWindowTitle + " [" + Process.Id + "]" + (Loaded ? " *" : "");
			}
		}
		public ProcessWrapper(Process p) { Process = p; }
	}

	public static class BindingListExtensions
	{
		public static void RemoveAll<T>(this BindingList<T> list, Predicate<T> pred)
		{
			for(int i = 0; i < list.Count; i++)
			{
				if(pred(list[i]))
					list.RemoveAt(i);
			}
		}
		public static bool Exists<T>(this BindingList<T> list, Predicate<T> pred)
		{
			foreach(T obj in list)
				if(pred(obj))
					return true;
			return false;
		}
	}

}
