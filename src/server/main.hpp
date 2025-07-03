#pragma once

class QuantumStreamer final : public Poco::Util::ServerApplication
{
protected:
	void initialize(Application& self) override;

	int main(const std::vector<std::string>& args) override;

private:
	void setupLogger() const;
	static void setupConsole();
};
