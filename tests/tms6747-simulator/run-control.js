importPackage(Packages.com.ti.debug.engine.scripting);
importPackage(Packages.com.ti.ccstudio.scripting.environment);
var base = "C:/Users/GRA/Documents/ChatGPT/Shalimar-ChatGPT/ShalimarCompiler/";
var script = ScriptingEnvironment.instance();
script.setScriptTimeout(20000);
var server = null;
var session = null;
try {
    server = script.getServer("DebugServer.1");
    server.setConfig(base + "tests/tms6747-simulator/c6747.ccxml");
    session = server.openSession(".*");
    session.target.connect();
    print("CONNECTED C6747 Device Functional Simulator");
    session.memory.loadProgram(base + "build/tms6747/control.out");
    var address = session.symbol.getAddress("halt_here");
    session.breakpoint.add(address);
    session.target.run();
    var actual = session.expression.evaluate("probe_result");
    print("TI CONTROL RESULT=" + actual + " EXPECTED=42");
    if (actual != 42) throw new Error("Simulator control returned wrong result");
    print("PASS simulator control; this is NOT Shalimar-generated code");
} catch (error) {
    print("FAIL " + error);
    if (server != null) server.stop();
    java.lang.System.exit(1);
}
if (session != null) session.terminate();
if (server != null) server.stop();
java.lang.System.exit(0);
