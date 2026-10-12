/* CCS 5.5 DSS: arguments are project root, generated .out, expected result, kind. */
importPackage(Packages.com.ti.debug.engine.scripting);
importPackage(Packages.com.ti.ccstudio.scripting.environment);
var base = String(arguments[0]);
var executable = String(arguments[1]);
var expected = Number(arguments[2]);
var realResult = arguments.length > 3 && String(arguments[3]) == "real";
if (arguments.length < 3) throw new Error("Arguments: project-root executable expected-result");
var script = ScriptingEnvironment.instance();
script.setScriptTimeout(30000);
var server = null;
var session = null;
try {
    server = script.getServer("DebugServer.1");
    server.setConfig(base + "/tests/tms6747-simulator/c6747.ccxml");
    session = server.openSession(".*");
    session.target.connect();
    session.memory.loadProgram(executable);
    session.breakpoint.add(session.symbol.getAddress("halt_here"));
    session.target.run();
    var actual = Number(session.memory.readData(Memory.Page.PROGRAM, session.symbol.getAddress("probe_result"), 32, 1, true)[0]);
    if (realResult) {
        var words = session.memory.readData(Memory.Page.PROGRAM, session.symbol.getAddress("probe_real"), 32, 2, false);
        var lo = Number(words[0]), hi = Number(words[1]);
        if (lo < 0) lo += 4294967296;
        if (hi < 0) hi += 4294967296;
        var low = lo.toString(16);
        while (low.length < 8) low = "0" + low;
        var high = hi.toString(16);
        var bits = new java.math.BigInteger(high + low, 16).longValue();
        actual = java.lang.Double.longBitsToDouble(bits);
    }
    print("SHALIMAR DSP RESULT=" + actual + " EXPECTED=" + expected);
    if (actual != expected) throw new Error("Incorrect Shalimar DSP result");
    print("PASS: Shalimar-generated DSP function executed in CCS simulator");
} catch (e) {
    print("FAIL: " + e);
    if (server != null) server.stop();
    java.lang.System.exit(1);
}
if (session != null) session.terminate();
if (server != null) server.stop();
java.lang.System.exit(0);
