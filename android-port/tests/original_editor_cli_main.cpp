// Test-only Android CLI launcher. The entire editor remains in libaseprite.so.
// This executable is never staged in the application or distributed.
extern int app_main(int argc, char** argv);
int main(int argc, char** argv) { return app_main(argc, argv); }
