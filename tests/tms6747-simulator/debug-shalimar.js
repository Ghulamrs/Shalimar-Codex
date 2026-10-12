importPackage(Packages.com.ti.debug.engine.scripting);
importPackage(Packages.com.ti.ccstudio.scripting.environment);
var base = String(arguments[0]), executable = String(arguments[1]), source = String(arguments[2]);
var kind = arguments.length > 3 ? String(arguments[3]) : "int";
var realCase = kind.indexOf("real") >= 0;
var tenCase = kind.indexOf("ten-") == 0;
var script = ScriptingEnvironment.instance(); script.setScriptTimeout(30000);
var server = null, session = null;
try {
    server = script.getServer("DebugServer.1");
    server.setConfig(base + "/tests/tms6747-simulator/c6747.ccxml");
    session = server.openSession(".*"); session.target.connect();
    session.memory.loadProgram(executable);
    var breakpoint = session.breakpoint.add(source, 3);
    session.target.run();
    var pc = session.memory.readRegister("PC");
    print("SOURCE BREAKPOINT PC=" + pc);
    if (tenCase) {
        var scale = realCase ? 2 : 1;
        if (session.expression.evaluate("sizeof(a)") != (realCase ? 8 : 4) ||
            session.expression.evaluate("(int)(a*" + scale + ")") != (realCase ? 3 : 1) ||
            session.expression.evaluate("(int)(j*" + scale + ")") != (realCase ? 21 : 10) ||
            session.expression.evaluate("(int)(value*" + scale + ")") != (realCase ? 120 : 55))
            throw new Error("Ten-argument parameter/local inspection failed");
        print("VARIABLES ten arguments and local verified");
    } else if (realCase) {
        // CCS 5.5's evaluator miscompares decimal floating literals; use exact
        // integer scaling and independently inspect the IEEE-754 memory words.
        if (session.expression.evaluate("sizeof(x)") != 8 || session.expression.evaluate("sizeof(value)") != 8 ||
            session.expression.evaluate("(int)(x*4)") != 6 || session.expression.evaluate("(int)(value*4)") != -3 ||
            session.expression.evaluate("*(unsigned int*)&x") != 0 || session.expression.evaluate("*((unsigned int*)&x+1)") != 1073217536 ||
            session.expression.evaluate("*(unsigned int*)&value") != 0 || session.expression.evaluate("*((unsigned int*)&value+1)") != 3219652608)
            throw new Error("Incorrect real parameter/local types or locations");
        print("VARIABLES real x=1.5 value=-0.75; types verified");
    } else {
        var x = session.expression.evaluate("x"), y = session.expression.evaluate("y"), value = session.expression.evaluate("value");
        print("VARIABLES x=" + x + " y=" + y + " value=" + value);
        if (x != 17 || y != 25 || value != 42 || session.expression.evaluate("sizeof(x) == 4 && sizeof(value) == 4") != 1)
            throw new Error("Incorrect parameter/local DWARF types or locations");
    }
    var stack = String(session.callStack.getStackTrace());
    print("STACK\n" + stack);
    if (stack.indexOf(tenCase ? "sum10" : realCase ? "subtract" : "plus") < 0 || stack.indexOf(":3 PC") < 0 ||
        stack.indexOf("answer") < 0 || stack.indexOf("main") < 0 || stack.indexOf("_c_int00") < 0 ||
        stack.indexOf("symbol is not available") >= 0 || stack.indexOf("entry point was reached") < 0)
        throw new Error("Incorrect DSP call-stack unwind");
    session.breakpoint.remove(breakpoint);
    session.target.sourceStep.out();
    var afterStep = String(session.callStack.getStackTrace());
    print("SOURCE STEP OUT\n" + afterStep);
    if (afterStep.split("\n")[0].indexOf("answer") < 0) throw new Error("Source step-out failed");
    session.breakpoint.add(session.symbol.getAddress("halt_here"));
    session.target.run();
    var expected = tenCase ? (realCase ? 240 : 55) : (realCase ? -3 : 42);
    if (realCase ? session.expression.evaluate("(int)(probe_real*4)") != expected : session.expression.evaluate("probe_result") != expected)
        throw new Error("Incorrect result after debugger continuation");
    print("PASS: source breakpoint, parameters, local, call stack, continuation");
} catch (error) {
    print("FAIL " + error);
    if (server != null) server.stop();
    java.lang.System.exit(1);
}
session.terminate(); server.stop(); java.lang.System.exit(0);
