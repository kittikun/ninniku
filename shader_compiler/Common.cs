using CommandLine;
using System.Collections.Generic;

namespace shader_compiler
{
    public enum ShaderType
    {
        // should match ninniku:EShaderType for simplicity
        ComputeShader,

        RootSignature,
        PixelShader,
        VertexShader
    }

    public struct ShaderComponent
    {
        public string entry_;
        public string path_;
        public ShaderType type_;
    }

    public class Options
    {
        [Option('d', "dxcPath", Required = true, HelpText = "Path to dxc.exe")]
        public string CompilerPath { get; set; }

        [Option('c', "config", Required = true, HelpText = "Configuration (Debug/Release) to build")]
        public string Configuration { get; set; }

        [Option('i', "input", Required = true, HelpText = "Input shaders TOC file")]
        public string InputPath { get; set; }

        public bool IsDebug { get; set; }

        [Option('o', "outDir", Required = true, HelpText = "Output folder")]
        public string OutDir { get; set; }
    }

    public class PipelineState
    {
        public Dictionary<ShaderType, ShaderComponent> components_ = new Dictionary<ShaderType, ShaderComponent>();
        public string name_;
    }

    /// <summary>
    /// Root signatures can be shared across pipeline states
    /// </summary>
    public class RootSignatures
    {
        public static Dictionary<string, ShaderComponent> signatures_ = new Dictionary<string, ShaderComponent>();
    }
}