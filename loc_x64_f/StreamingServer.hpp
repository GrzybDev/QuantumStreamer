#pragma once

class StreamingServer : public Poco::Util::ServerApplication
{
protected:
	VOID initialize(Application& self) override;
	VOID uninitialize() override;

	INT main(const std::vector<std::string>& args) override;

private:
	VOID initLoggers();
	VOID createConsole();
};
