$ErrorActionPreference='Stop'
$taskFixtureName='TaskManagerX64-Test-'+[Guid]::NewGuid().ToString('N')
$taskFixtureRoot=$null
$taskFixtureCreated=$false
try {
    $service=New-Object -ComObject Schedule.Service
    $service.Connect()
    $taskFixtureRoot=$service.GetFolder('\')
    $definition=$service.NewTask(0)
    $definition.RegistrationInfo.Description='Temporary TaskManagerX64 API test; no triggers, never executed.'
    $definition.Settings.Enabled=$true
    $definition.Settings.ExecutionTimeLimit='PT1M'
    $definition.Principal.UserId=[Security.Principal.WindowsIdentity]::GetCurrent().Name
    $definition.Principal.LogonType=3
    $action=$definition.Actions.Create(0)
    $action.Path=Join-Path $PSScriptRoot 'outputs/test-target.exe'
    try {
        $null=$taskFixtureRoot.RegisterTaskDefinition($taskFixtureName,$definition,2,$definition.Principal.UserId,$null,3)
        $taskFixtureCreated=$true
    } catch {
        if($_.Exception.HResult -eq -2147024891) {
            'SKIP: task creation denied for the current token; read-only scheduler enumeration is covered by model-tests.'
            return
        }
        throw
    }
    $env:TMX_TEST_TASK='\'+$taskFixtureName
    & (Join-Path $PSScriptRoot 'outputs/task-mutation-test.exe')
    if($LASTEXITCODE){throw "MASM task mutation test failed: $LASTEXITCODE"}
    if(-not $taskFixtureRoot.GetTask($taskFixtureName).Enabled){throw 'The fixture task was not restored.'}
    'PASS: MASM Task Scheduler disable/enable round trip, verified through independent COM query.'
} finally {
    if($taskFixtureCreated){$taskFixtureRoot.DeleteTask($taskFixtureName,0)}
    Remove-Item Env:\TMX_TEST_TASK -ErrorAction SilentlyContinue
}
