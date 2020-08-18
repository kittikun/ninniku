using System;
using System.IO;

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
    }
}