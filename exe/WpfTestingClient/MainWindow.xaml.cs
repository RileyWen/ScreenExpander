using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.Pipes;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace WpfTestingClient
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            m_pipe = new NamedPipeClientStream(".", "RileyNamedPipe",
                PipeAccessRights.ReadData | PipeAccessRights.WriteAttributes,
                PipeOptions.None,
                System.Security.Principal.TokenImpersonationLevel.None,
                System.IO.HandleInheritability.None);


            /*            object obj = FindName("Screen");
                        if (obj is Image)
                        {
                            Debug.WriteLine("[Riley] Screen Found!");

                            Image Screen = obj as Image;

                            BitmapImage bitmap = new BitmapImage();
                            bitmap.BeginInit();
                            bitmap.UriSource = new Uri(@"http://static01.coloros.com/bbs/data/attachment/forum/201503/17/092752ywabeihuha0iza0d.jpg");
                            bitmap.EndInit();

                            Screen.Source = bitmap;
                        }
                        else
                        {
                            Debug.WriteLine("[Riley] Screen Not Found!");
                        }
            */
            new Thread(new ThreadStart(ImageUpdateThread)).Start();

            Debug.WriteLine("Init finished.");
        }

        private void ImageUpdateThread()
        {
            Debug.WriteLine("New Image thread started.");

            byte[] ImageBytes = new byte[1920 * 1080 * 32 / 8 + 8];

            try
            {
                m_pipe.Connect();
            }
            finally { }

            m_pipe.ReadMode = PipeTransmissionMode.Message;

            try
            {
                while (true)
                {
                    int BytesRead = m_pipe.Read(ImageBytes, 0, ImageBytes.Length);

                    Debug.WriteLine("Receive %d bytes", BytesRead);
                    ImageFrame frame = ImageFrame.FromBytes(ImageBytes);

/*                    BitmapSource bmSource = BitmapSource.Create(
                        (int)frame.Header.dwWidth, (int)frame.Header.dwWidth, 200, 200,
                        PixelFormats.Bgra32, BitmapPalettes.Halftone256,
                        frame.pData, (int)frame.Header.dwWidth);

                    Screen.Source = bmSource;
*/
                    Debug.WriteLine("Receive a frame");
                }
            }
            finally
            {
                Debug.WriteLine("ImageUpdateThread Exit");
            }
        }

        private void Button_Connect_Click(object sender, RoutedEventArgs e)
        {

        }

        NamedPipeClientStream m_pipe;
    }
}
