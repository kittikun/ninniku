using System;
using System.IO;
using CommandLine;

namespace shader_compiler
{
    internal class Program
    {
        private static void Main(string[] args)
        {
            var projectDir = Directory.GetParent(Environment.CurrentDirectory).Parent.Parent.FullName;
            var dataDir = Path.Combine(projectDir, "data");

            var parser = new Parser(dataDir);

            parser.Parse("shaders.xml");
        }

        public class Options
        {
            [Option('v', "verbose", Required = false, HelpText = "Set output to verbose messages.")]
            public bool Verbose { get; set; }
        }
    }
}