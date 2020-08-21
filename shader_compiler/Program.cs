using System;
using System.IO;
using CommandLine;

namespace shader_compiler
{
    internal class Program
    {
        private static void Main(string[] args)
        {
            CommandLine.Parser.Default.ParseArguments<Options>(args)
                  .WithParsed<Options>(o =>
                  {
                      if (o.InputPath.Length > 0)
                      {
                          if (!File.Exists(o.CompilerPath))
                              throw new System.IO.FileNotFoundException(o.CompilerPath);

                          if (!File.Exists(o.InputPath))
                              throw new System.IO.FileNotFoundException(o.InputPath);

                          if (!Directory.Exists(o.OutDir))
                          {
                              Directory.CreateDirectory(o.OutDir);
                          }

                          if ((o.Configuration != "Debug") && (o.Configuration != "Release") && (o.Configuration != "Trace"))
                              throw new ArgumentException("Configuration must be either \"Debug\", \"Release\", \"Trace\"");

                          o.IsDebug = o.Configuration == "Debug";

                          var baseDir = Path.GetDirectoryName(o.InputPath);
                          var filename = Path.GetFileName(o.InputPath);

                          // Parse list of files to compile
                          var parser = new Parser(baseDir, o);

                          var shaders = parser.Parse(filename);

                          var compiler = new Compiler(o);

                          compiler.CompileShaders(shaders);

                          // need to clean up temp files
                          foreach (var shader in shaders)
                          {
                              if (shader.type_ == ShaderType.RootSignature)
                              {
                                  // somehow deleting the temp files make Appveyor fail..
                                  if (Environment.GetEnvironmentVariable("APPVEYOR") == null)
                                      File.Delete(shader.path_);
                              }
                          }
                      }
                  });
        }
    }
}