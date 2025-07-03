#include "../Pipes/PipesHandler.hpp"
#include "../Miscellaneous/Execution/Execution.hpp"
#include "../Miscellaneous/NeedUpdating/Offsets.hpp"

namespace Communication {

    NamedPipe::NamedPipe() : hPipe(INVALID_HANDLE_VALUE), running(false) {}

    NamedPipe::~NamedPipe() {
        Stop();
    }

    bool NamedPipe::Start() {
        running = true;

        while (running) {
            hPipe = CreateNamedPipeA(
                PIPE_NAME,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                BUFFER_SIZE,
                BUFFER_SIZE,
                0,
                nullptr
            );

            if (hPipe == INVALID_HANDLE_VALUE) {
                return false;
            }

            BOOL connected = ConnectNamedPipe(hPipe, nullptr) ?
                TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

            if (connected) {
                HandleClient();
            }

            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }

        return true;
    }

    void NamedPipe::Stop() {
        running = false;
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
        if (PipeThread.joinable()) {
            PipeThread.join();
        }
    }

    void NamedPipe::HandleClient() {
        char buffer[BUFFER_SIZE];
        DWORD bytesRead;
        DWORD bytesWritten;

        while (running) {
            BOOL success = ReadFile(
                hPipe,
                buffer,
                BUFFER_SIZE - 1,
                &bytesRead,
                nullptr
            );

            if (!success || bytesRead == 0) {
                break;
            }

            buffer[bytesRead] = '\0';
            std::string luaScript(buffer, bytesRead);

            try {
                Execution->Execute(Globals::ExploitThread, luaScript);

                const char* response = "Script executed successfully";
                WriteFile(
                    hPipe,
                    response,
                    strlen(response),
                    &bytesWritten,
                    nullptr
                );
            }
            catch (const std::exception& e) {
                std::string errorMsg = "Execution error: " + std::string(e.what());
                WriteFile(
                    hPipe,
                    errorMsg.c_str(),
                    errorMsg.length(),
                    &bytesWritten,
                    nullptr
                );
            }

            FlushFileBuffers(hPipe);
        }
    }


    int NamedPipe::InitializeNamePipe() {

        Execution->Execute(Globals::ExploitThread, "warn('[Cravex] Pipe System Loaded.')");
        NamedPipe server;

        std::thread serverThread([&server]() {
            server.Start();
            });



        if (serverThread.joinable()) {
            serverThread.join();
        }


        return 0;
    }

}