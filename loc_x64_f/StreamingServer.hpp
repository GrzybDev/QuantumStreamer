#pragma once

class StreamingServer : public Poco::Util::ServerApplication
{
protected:
	void initialize(Application& self) override;
	void uninitialize() override;

	int main(const std::vector<std::string>& args) override;

private:
	void initLoggers() const;
	static void createConsole();
};
