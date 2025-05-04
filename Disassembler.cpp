#include "Disassembler.h"
#include "CodeFormatter.h"
#include "DisassemblyWriter.h"
#include "LabelGenerator.h"
#include "MemoryAnalyzer.h"
#include "SIDLoader.h"
#include "cpu6510.h"

namespace sidblaster {

    Disassembler::Disassembler(const CPU6510& cpu, const SIDLoader& sid)
        : cpu_(cpu),
        sid_(sid) {
        initialize();
    }

    Disassembler::~Disassembler() = default;

    void Disassembler::initialize() {
        util::Logger::debug("Initializing disassembler...");

        // Create memory analyzer but don't analyze yet
        analyzer_ = std::make_unique<MemoryAnalyzer>(
            cpu_.getMemory(),
            cpu_.getMemoryAccess(),
            sid_.getLoadAddress(),
            sid_.getLoadAddress() + sid_.getDataSize()
        );

        // Create label generator
        labelGenerator_ = std::make_unique<LabelGenerator>(
            *analyzer_,
            sid_.getLoadAddress(),
            sid_.getLoadAddress() + sid_.getDataSize()
        );

        // Create code formatter
        formatter_ = std::make_unique<CodeFormatter>(
            cpu_,
            *labelGenerator_,
            cpu_.getMemory()
        );

        // Create disassembly writer
        writer_ = std::make_unique<DisassemblyWriter>(
            cpu_,
            sid_,
            *analyzer_,
            *labelGenerator_,
            *formatter_
        );

        // Set up indirect read callback
        const_cast<CPU6510&>(cpu_).setOnIndirectReadCallback([this](u16 pc, u8 zpAddr, u16 effectiveAddr) {
            if (writer_) {
                writer_->addIndirectAccess(pc, zpAddr, effectiveAddr);
            }
            });

        util::Logger::debug("Disassembler initialization complete");
    }

    int Disassembler::generateAsmFile(
        const std::string& outputPath,
        u16 sidLoad,
        u16 sidInit,
        u16 sidPlay) {

        if (!analyzer_ || !labelGenerator_ || !formatter_ || !writer_) {
            util::Logger::error("Disassembler not properly initialized");
            return -1;
        }

        // NOW perform the analysis AFTER all CPU execution is complete
        util::Logger::debug("Performing memory analysis...");
        analyzer_->analyzeExecution();
        analyzer_->analyzeAccesses();
        analyzer_->analyzeData();

        // Process any detected indirect accesses to identify relocation entries
        util::Logger::debug("Processing indirect memory accesses...");
        writer_->processIndirectAccesses();

        // Generate labels based on the analysis
        util::Logger::debug("Generating labels...");
        labelGenerator_->generateLabels();

        // Apply any pending subdivisions to data blocks
        labelGenerator_->applySubdivisions();

        // Generate the assembly file
        return writer_->generateAsmFile(outputPath, sidLoad, sidInit, sidPlay);
    }

    void Disassembler::setIndirectReadCallback(
        std::function<void(u16 pc, u8 zpAddr, u16 effectiveAddr)> callback) {

        indirectReadCallback_ = std::move(callback);

        // If writer exists, set up the indirect access handler
        if (writer_ && indirectReadCallback_) {
            indirectReadCallback_ = [this](u16 pc, u8 zpAddr, u16 effectiveAddr) {
                // Process in the writer
                writer_->addIndirectAccess(pc, zpAddr, effectiveAddr);

                // Forward to user callback if any
                if (this->indirectReadCallback_) {
                    this->indirectReadCallback_(pc, zpAddr, effectiveAddr);
                }
                };
        }
    }

} // namespace sidblaster