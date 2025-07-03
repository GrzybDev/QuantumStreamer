#pragma once

class QuantumStreamer : public Poco::Util::ServerApplication
{
protected:
	void initialize(Application& self) override;

private:
	void setupLogger() const;
	static void setupConsole();
};
