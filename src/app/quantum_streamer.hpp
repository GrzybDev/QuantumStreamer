#pragma once

#include <Poco/Util/Application.h>

#include "settings.hpp"

namespace qs
{
	/// Reads the game's configuration and serves the episodes until the game exits.
	class QuantumStreamer final : public Poco::Util::Application
	{
	protected:
		void initialize(Application& self) override;
		int main(const std::vector<std::string>& args) override;

	private:
		Settings settings_{};
	};
}
